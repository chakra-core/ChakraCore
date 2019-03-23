'use strict';

const log = require('npmlog');
const tmp = require('tmp');
const path = require('path');
const Bluebird = require('bluebird');

const template = require('./strutils').template;
const ProcessCtl = require('./process-ctl');

// setup graceful cleanup: removes the created directories when an uncaught exception occurs.
tmp.setGracefulCleanup();

module.exports = class Launcher {
  constructor(name, settings, config) {
    this.name = name;
    this.config = config;
    this.settings = settings;
    this.setupDefaultSettings();
    this.id = settings.id || String(Math.floor(Math.random() * 10000));

    this.processCtl = new ProcessCtl(name, config);
  }

  setupDefaultSettings() {
    let settings = this.settings;
    if (settings.protocol === 'tap' && !('hide_stdout' in settings)) {
      settings.hide_stdout = true;
    }
  }

  isProcess() {
    return this.settings.protocol !== 'browser';
  }

  protocol() {
    return this.settings.protocol || 'process';
  }

  commandLine() {
    if (this.settings.command) {
      return '"' + this.settings.command + '"';
    } else if (this.settings.exe) {
      return '"' + this.settings.exe +
        ' ' + this.getArgs().join(' ') + '"';
    }
  }

  start() {
    return this.launch();
  }

  launch() {
    const settings = this.settings;
    this.setupBrowserTmpDir();

    return Bluebird.resolve().asCallback(() => {
      if (settings.setup) {
        return Bluebird.fromCallback(setupCallback => {
          settings.setup.call(this, this.config, setupCallback);
        });
      }

      return Bluebird.resolve();
    }).then(() => this.doLaunch());
  }

  doLaunch() {
    let settings = this.settings;
    let options = {};

    if (settings.cwd) {
      options.cwd = settings.cwd;
    }

    if (settings.exe) {
      let args = this.getArgs();
      args = this.template(args);

      return this.processCtl.spawn(settings.exe, args, options);
    } else if (settings.command) {
      let cmd = this.template(settings.command);
      log.info('cmd: ' + cmd);

      return this.processCtl.exec(cmd, options);
    } else {
      return Bluebird.reject(new Error('No command or exe/args specified for launcher ' + this.name));
    }
  }

  getId() {
    return this.isProcess() ? -1 : this.id;
  }

  getUrl() {
    let baseUrl = this.config.get('url');
    let testPage = this.settings.test_page;
    let id = this.getId();

    return baseUrl + id + (testPage ? '/' + testPage : '');
  }

  getArgs() {
    let settings = this.settings;
    let url = this.getUrl();
    let args = [url];
    if (settings.args instanceof Array) {
      args = settings.args.concat(args);
    } else if (settings.args instanceof Function) {
      args = settings.args.call(this, this.config, url);
    }
    return args;
  }

  template(thing) {
    if (Array.isArray(thing)) {
      return thing.map(this.template, this);
    } else {
      let params = {
        cwd: this.config.cwd(),
        url: this.getUrl(),
        baseUrl: this.config.get('url'),
        port: this.config.get('port'),
        testPage: this.settings.test_page || '',
        id: this.getId()
      };
      return template(thing, params);
    }
  }

  setupBrowserTmpDir() {
    const userDataDir = this.config.getUserDataDir();
    const tmpPath = path.join(userDataDir, 'testem-' + this.id);

    this.browserTmpDirectory = tmp.dirSync({
      template: `${tmpPath}-XXXXXX`,
      unsafeCleanup: true
    });
  }

  browserTmpDir() {
    if (!this.browserTmpDirectory) {
      setupBrowserTmpDir();
    }

    return this.browserTmpDirectory.name;
  }
};
