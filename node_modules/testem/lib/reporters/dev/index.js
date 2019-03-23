'use strict';

/*

appview.js
==========

The actual AppView. This encapsulates the entire UI.

*/

const StyledString = require('styled_string');
const log = require('npmlog');
const Backbone = require('backbone');

const View = require('./view');
const tabs = require('./runner_tabs');
const constants = require('./constants');
const RunnerTab = tabs.RunnerTab;
const RunnerTabs = tabs.RunnerTabs;
const Screen = require('./screen');
const pad = require('../../strutils').pad;
const ErrorMessagesPanel = require('./error_messages_panel');
const Runner = require('./runner');

module.exports = View.extend({
  defaults: {
    currentTab: 0,
    atLeastOneRunner: false
  },
  initialize(silent, out, config, app, screen) {
    this.name = 'Testem';
    this.config = config;
    this.app = app;
    this.viewRunners = new Backbone.Collection();
    this.x = {};

    screen = screen || new Screen();

    this.set('screen', screen);

    this.on('ctrl-c', () => app.exit());

    this.initCharm();
    this.on('inputChar', this.onInputChar.bind(this));

    let runnerTabs = this.runnerTabs = new RunnerTabs([], {
      appview: this,
      screen: screen
    });
    this.set({
      runnerTabs: runnerTabs
    });
    runnerTabs.on('add', () => runnerTabs.render());
    this.app.on('runnerAdded', runner =>  this.runnerAdded(runner));
    this.app.on('runnerRemoved', runner => this.runnerRemoved(runner));

    runnerTabs.on('add', () => runnerTabs.render());
    this.on('change:atLeastOneRunner', (atLeastOne) => {
      if (atLeastOne && this.get('currentTab') < 0) {
        this.set('currentTab', 0);
      }
      this.renderMiddle();
      this.renderBottom();
    });
    this.on('change:lines change:cols', () => {
      this.render();
    });

    this.errorMessagesPanel = new ErrorMessagesPanel({
      appview: this,
      text: '',
      screen: screen
    });
    this.errorMessagesPanel.on('change:text', (m, text) => {
      this.set('isPopupVisible', !!text);
    });
    this.startMonitorTermSize();
  },

  runnerAdded(runner) {
    let viewRunner = new Runner(runner);
    this.viewRunners.push(viewRunner);

    let idx = this.viewRunners.length - 1;
    this.x[runner.launcherId] = viewRunner;

    log.info('runnerAdded', runner.name(), runner.launcherId);

    let tab = new RunnerTab({
      runner: viewRunner,
      index: idx,
      appview: this,
      screen: this.get('screen')
    });
    this.runnerTabs.push(tab);
    this.set('atLeastOneRunner', this.viewRunners.length > 0);
  },

  runnerRemoved(runner) {
    log.info('runnerRemoved');

    this.viewRunners.remove(this.x[runner.launcherId]);
    this.set('atLeastOneRunner', this.viewRunners.length > 0);
  },

  _showError(titleText, err) {
    let title = new StyledString(titleText + '\n ').foreground('red');

    if (err) {
      let errMsgs = new StyledString('\n' + err.name)
        .foreground('white')
        .concat(new StyledString('\n' + err.message).foreground('red'));

      title += errMsgs;
    }

    this.setErrorPopupMessage(title);

    if (err) {
      log.log('warn', titleText);
    } else {
      log.log('warn', titleText, {
        name: err.name,
        message: err.message
      });
    }
  },

  onInputChar(chr, i) {
    if (chr === 'q') {
      log.info('Got keyboard Quit command');
      this.app.exit();
    } else if (i === 13) { // ENTER
      log.info('Got keyboard Start Tests command');
      this.app.triggerRun('Triggered manually by pressing enter');
    } else if (chr === 'p') {
      this.app.paused = !this.app.paused;
      this.renderBottom();
    }
  },

  initCharm() {
    let screen = this.get('screen');
    screen.reset();
    screen.erase('screen');
    screen.cursor(false);
    screen.on('data', this.onScreenData.bind(this));
    screen.removeAllListeners('^C');
    screen.on('^C', () => this.trigger('ctrl-c'));
  },

  startMonitorTermSize() {
    this.updateScreenDimensions();
    process.stdout.on('resize', () => {
      let cols = process.stdout.columns;
      let lines = process.stdout.rows;
      if (cols !== this.get('cols') || lines !== this.get('lines')) {
        this.updateScreenDimensions();
      }
    });
  },
  updateScreenDimensions() {
    let screen = this.get('screen');
    let cols = process.stdout.columns;
    let lines = process.stdout.rows;
    screen.enableScroll(constants.LogPanelUnusedLines, lines - 1);
    this.set({
      cols: cols,
      lines: lines
    });
    this.updateErrorMessagesPanelSize();
  },
  updateErrorMessagesPanelSize() {
    this.errorMessagesPanel.set({
      line: 2,
      col: 4,
      width: this.get('cols') - 8,
      height: this.get('lines') - 4
    });
  },
  render() {
    this.renderTop();
    if (!this.get('atLeastOneRunner')) {
      this.renderMiddle();
    }
    this.renderBottom();
  },
  renderTop() {
    if (this.isPopupVisible()) {
      return;
    }

    let screen = this.get('screen');
    let url = this.config.get('url');
    let cols = this.get('cols');
    screen
      .position(0, 1)
      .write(pad('TEST\u0027EM \u0027SCRIPTS!', cols, ' ', 1))
      .position(0, 2)
      .write(pad('Open the URL below in a browser to connect.', cols, ' ', 1))
      .position(0, 3)
      .display('underscore')
      .write(url, cols, ' ', 1)
      .display('reset')
      .position(url.length + 1, 3)
      .write(pad('', cols - url.length, ' ', 1));

  },
  renderMiddle() {
    if (this.isPopupVisible()) {
      return;
    }
    if (this.viewRunners.length > 0) {
      return;
    }

    let screen = this.get('screen');
    let lines = this.get('lines');
    let cols = this.get('cols');
    let textLineIdx = Math.floor(lines / 2 + 2);
    for (let i = constants.LogPanelUnusedLines; i < lines; i++) {
      let text = (i === textLineIdx ? 'Waiting for runners...' : '');
      screen
        .position(0, i)
        .write(pad(text, cols, ' ', 2));
    }
  },
  renderBottom() {
    if (this.isPopupVisible()) {
      return;
    }

    let screen = this.get('screen');
    let cols = this.get('cols');
    let pauseStatus = this.app.paused ? '; p to unpause (PAUSED)' : '; p to pause';

    let msg = (
      !this.get('atLeastOneRunner') ?
        'q to quit' :
        'Press ENTER to run tests; q to quit'
    );
    msg = '[' + msg + pauseStatus + ']';
    screen
      .position(0, this.get('lines'))
      .write(pad(msg, cols - 1, ' ', 1));
  },
  runners() {
    return this.viewRunners;
  },
  currentRunnerTab() {
    let idx = this.get('currentTab');
    return this.runnerTabs.at(idx);
  },

  onScreenData(buf) {
    try {
      let chr = String(buf).charAt(0);
      let i = chr.charCodeAt(0);
      let key = (buf[0] === 27 && buf[1] === 91) ? buf[2] : null;
      let currentRunnerTab = this.currentRunnerTab();
      let splitPanel = currentRunnerTab && currentRunnerTab.splitPanel;

      //log.info([buf[0], buf[1], buf[2]].join(','))
      if (key === 67) { // right arrow
        this.nextTab();
      } else if (key === 68) { // left arrow
        this.prevTab();
      } else if (key === 66) { // down arrow
        splitPanel.scrollDown();
      } else if (key === 65) { // up arrow
        splitPanel.scrollUp();
      } else if (chr === '\t') {
        splitPanel.toggleFocus();
      } else if (chr === ' ' && splitPanel) {
        splitPanel.pageDown();
      } else if (chr === 'b') {
        splitPanel.pageUp();
      } else if (chr === 'u') {
        splitPanel.halfPageUp();
      } else if (chr === 'd') {
        splitPanel.halfPageDown();
      }
      this.trigger('inputChar', chr, i);
    } catch (e) {
      log.error('In onInputChar: ' + e + '\n' + e.stack);
    }
  },
  nextTab() {
    let currentTab = this.get('currentTab');
    currentTab++;
    if (currentTab >= this.runners().length) {
      currentTab = 0;
    }

    this.set('currentTab', currentTab);
  },
  prevTab() {
    let currentTab = this.get('currentTab');
    currentTab--;
    if (currentTab < 0) {
      currentTab = this.runners().length - 1;
    }

    this.set('currentTab', currentTab);
  },
  setErrorPopupMessage(msg) {
    this.errorMessagesPanel.set('text', msg);
  },
  clearErrorPopupMessage() {
    this.errorMessagesPanel.set('text', '');
    this.render();
  },
  isPopupVisible() {
    return !!this.get('isPopupVisible');
  },
  setRawMode() {
    if (process.stdin.isTTY) {
      process.stdin.setRawMode(false);
    }
  },

  report(browserName, result) {
    if (isTestemItself(result)) {
      return this._showError('Testem error', result.error);
    }

    let runner = this.x[result.launcherId];
    if (!runner) {
      return;
    }

    runner.report(result);

    if (result.logs) {
      result.logs.forEach(log => runner.get('messages').push(log));
    }
  },

  onStart(browserName, opts) {
    if (isTestemItself(opts)) {
      return this.clearErrorPopupMessage();
    }

    let runner = this.x[opts.launcherId];

    if (!runner) {
      return;
    }

    log.info(browserName, 'onStart');

    runner.onStart();
  },

  onEnd(browserName, opts) {
    if (isTestemItself(opts)) {
      return;
    }

    let runner = this.x[opts.launcherId];
    if (!runner) {
      return;
    }

    log.info(browserName, 'onEnd');

    runner.onEnd();
  },

  finish() {
    this.cleanup();
  },

  cleanup(cb) {
    let screen = this.get('screen');
    screen.display('reset');
    screen.erase('screen');
    screen.position(0, 0);
    screen.enableScroll();
    screen.cursor(true);
    this.setRawMode(false);
    screen.destroy();
    if (cb) {
      cb();
    }
  }
});

function isTestemItself(opts) {
  return opts.launcherId === 0;
}
