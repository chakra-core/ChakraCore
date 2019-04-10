'use strict';

const log = require('npmlog');
const BrowserTapConsumer = require('../browser_tap_consumer');
const util = require('util');
const Bluebird = require('bluebird');

const toResult = require('./to-result');

module.exports = class BrowserTestRunner {
  constructor(launcher, reporter, index, singleRun, config) {
    this.launcher = launcher;
    this.reporter = reporter;
    this.running = false;
    this.config = config;
    this.index = index;
    this.launcherId = this.launcher.id;
    this.singleRun = singleRun;
    this.logs = [];
    this.currentTestContext = {};

    this.pendingTimer = undefined;
    this.onProcessExitTimer = undefined;
    this.exitRequested = false;
  }

  start(onFinish) {
    if (this.pending) {
      return;
    }

    this.finished = false;
    this.pending = true;

    return new Bluebird.Promise(resolve => {
      this.onFinish = resolve;

      if (this.socket) {
        this.socket.emit('start-tests');
      } else {
        this.launcher.start().then(browserProcess => {
          this.process = browserProcess;
          this.process.on('processExit', this.onProcessExit.bind(this));
          this.process.on('processError', this.onProcessError.bind(this));
          this.setupStartTimer();
        }).catch(err => {
          this.onProcessError(err);
        });
      }
    }).asCallback(onFinish);
  }

  stop(cb) {
    if (this.socket) {
      this.socket.emit('stop-run');
    }
    return Bluebird.resolve().asCallback(cb);
  }

  exit() {
    if (!this.process) {
      return Bluebird.resolve();
    }

    // mark that somebody (generally app.js) has requested that the
    // test runner exit. This is not an unexpected exit--this setting
    // is to tell the processExit handler that.
    this.exitRequested = true;

    log.info(`Closing browser ${this.name()}.`);
    return this.process.kill().then(() => {
      this.process = null;
    });
  }

  setupStartTimer() {
    this.startTimer = setTimeout(() => {
      if (this.finished || !this.pending) {
        return;
      }

      let err = new Error(
        `Browser failed to connect within ${this.launcher.config.get('browser_start_timeout')}s. testem.js not loaded?`
      );
      this.reportResults(err, 0);
    }, this.launcher.config.get('browser_start_timeout') * 1000);
  }

  tryAttach(browser, id, socket) {
    if (id !== this.launcherId) {
      return;
    }

    log.info('tryAttach', browser, id);

    if (this.startTimer) {
      clearTimeout(this.startTimer);
    }
    if (this.pendingTimer) {
      clearTimeout(this.pendingTimer);
    }

    this.pending = false;
    this.socket = socket;
    this.browser = browser;
    this.logs = [];

    this.onStart.call(this);

    socket.on('tests-start', this.onTestsStart.bind(this));
    socket.on('test-result', this.onTestResult.bind(this));
    socket.on('test-metadata', this.onTestMetadata.bind(this));
    socket.on('top-level-error', this.onGlobalError.bind(this));

    socket.on('browser-console', function(/* ...args */) {
      let args = Array.prototype.slice.call(arguments);
      let type = args.shift();
      let message = args.map(arg => util.inspect(arg, { depth: null })).join(' ');

      this.logs.push({
        type: type,
        text: `${message}\n`
      });
    }.bind(this));

    socket.on('disconnect', this.onDisconnect.bind(this));

    socket.on('all-test-results', this.onAllTestResults.bind(this));
    socket.on('after-tests-complete', this.onAfterTests.bind(this));

    const customBrowserSocketEvents = this.launcher.config.get('custom_browser_socket_events');

    if (customBrowserSocketEvents) {
      Object.keys(customBrowserSocketEvents).forEach((key) => {
        socket.on(key, customBrowserSocketEvents[key].bind(this));
      });
    }

    let tap = new BrowserTapConsumer(socket);
    tap.on('test-result', this.onTestResult.bind(this));
    tap.on('all-test-results', this.onAllTestResults.bind(this));
    tap.on('all-test-results', () => {
      this.socket.emit('tap-all-test-results');
    });
  }

  name() {
    return this.launcher.name;
  }

  reportResults(err, code, browserProcess) {
    browserProcess = browserProcess || this.process;

    let result = toResult(this.launcherId, err, code, browserProcess, this.config, this.currentTestContext);
    this.reporter.report(this.launcher.name, result);
    this.finish();
  }

  onTestsStart(testData) {
    if (testData) {
      this.currentTestContext = testData;
      this.currentTestContext.state = 'executing';
    }
  }

  onTestResult(result) {
    let errItems = (result.items || [])
      .filter(item => !item.passed);

    this.reporter.report(this.browser, {
      passed: !result.failed && !result.skipped,
      name: result.name,
      skipped: result.skipped,
      runDuration: result.runDuration,
      logs: this.logs,
      error: errItems[0],
      launcherId: this.launcherId,
      failed: result.failed,
      pending: result.pending,
      items: result.items,
      originalResultObj: result
    });
    this.logs = [];
    this.currentTestContext.state = 'complete';
  }

  onTestMetadata(tag, metadata) {
    this.reporter.reportMetadata(tag, metadata);
  }

  onStart() {
    this.reporter.onStart(this.browser, {
      launcherId: this.launcherId
    });
  }

  onEnd() {
    this.reporter.onEnd(this.browser, {
      launcherId: this.launcherId
    });
  }

  onAllTestResults() {
    log.info(`Browser ${this.name()} finished all tests.`, this.singleRun);
    this.onEnd();
  }

  onAfterTests() {
    this.finish();
  }

  onGlobalError(msg, url, line) {
    let message = `${msg} at ${url}, line ${line}\n`;
    this.logs.push({
      type: 'error',
      testContext: this.currentTestContext,
      text: message
    });

    if (this.currentTestContext && this.currentTestContext.name) {
      if (this.currentTestContext.state === 'executing') {
        message = `Global error: ${msg} at ${url}, line ${line}\n While executing test: ${this.currentTestContext.name}\n`;
      } else if (this.currentTestContext.state === 'complete') {
        message = `Global error: ${msg} at ${url}, line ${line}\n After execution of test: ${this.currentTestContext.name}\n`;
      }
    } else {
      message = `Global error: ${msg} at ${url}, line ${line}\n`;
    }

    let config = this.launcher.config;
    if (config.get('bail_on_uncaught_error')) {
      this.onTestResult.call(this, {
        failed: 1,
        name: message,
        logs: [],
        error: {}
      });
      this.onAllTestResults();
      this.onEnd.call(this);
    }
  }

  onDisconnect() {
    this.socket = null;
    if (this.finished) { return; }

    this.pending = true;
    let timeout  = this.launcher.config.get('browser_disconnect_timeout');
    this.pendingTimer = setTimeout(() => {
      if (this.finished) {
        return;
      }

      this.reportResults(new Error(`Browser timeout exceeded: ${timeout}s`), 0);
    }, timeout * 1000);
  }

  onProcessExit(code) {
    let browserProcess = this.process;
    this.process = null;
    if (this.finished) { return; }

    this.onProcessExitTimer = setTimeout(() => {
      if (this.finished) {
        return;
      }

      this.reportResults(new Error(this.exitRequested
                                   ? "Browser exited on request from test driver"
                                   : "Browser exited unexpectedly"),
                         code,
                         browserProcess);
    }, 1000);
  }

  onProcessError(err) {
    let browserProcess = this.process;
    this.process = null;

    if (this.finished) { return; }

    this.reportResults(err, 0, browserProcess);
  }

  finish() {
    if (this.finished) { return; }

    clearTimeout(this.pendingTimer);
    clearTimeout(this.onProcessExitTimer);

    this.finished = true;

    if (!this.singleRun) {
      if (this.onFinish) {
        this.onFinish();
      }
      return;
    }
    return this.exit().then(() => {
      // TODO: Not sure how this can happen, but sometimes onFinish is not defined
      if (this.onFinish) {
        this.onFinish();
      }
    });
  }
};
