'use strict';

const Bluebird = require('bluebird');
const log = require('npmlog');

const template = require('../strutils').template;
const ProcessCtl = require('../process-ctl');

module.exports = class HookRunner {
  constructor(config) {
    this.config = config;
  }

  run(hook, data, callback) {
    let hookCfg = this.config.get(hook);
    if (!hookCfg) {
      return Bluebird.resolve();
    }
    if (typeof hookCfg === 'function') {
      return Bluebird.fromCallback(callback => {
        hookCfg(this.config, data, callback);
      });
    }

    this.processCtl = new ProcessCtl(hook, this.config);

    let command;
    let exe;
    let args;
    let waitForText;
    let waitForTextTimeout;
    let badText;
    let badTextTimeout;

    if (typeof hookCfg === 'object') {
      command = hookCfg.command;
      exe = hookCfg.exe;
      args = hookCfg.args;
      waitForText = hookCfg.wait_for_text;
      waitForTextTimeout = hookCfg.wait_for_text_timeout;
      badText = hookCfg.bad_text;
      badTextTimeout = hookCfg.bad_text_timeout;
    } else if (typeof hookCfg === 'string') {
      command = hookCfg;
    }

    let options = {
      cwd: this.config.cwd()
    };

    let hookProcessPromise;
    if (command) {
      command = this.varsub(command, data);
      hookProcessPromise = this.processCtl.exec(command, options);
    } else if (exe) {
      args = this.varsub(args || []);
      hookProcessPromise = this.processCtl.spawn(exe, args, options);
      command = exe + ' ' + args.join(' ');
    } else {
      return Bluebird.reject(new Error('No command or exe/args specified for hook ' + hook));
    }

    waitForTextTimeout = waitForTextTimeout || 10000;
    badTextTimeout = badTextTimeout || waitForTextTimeout;

    let exited = false;
    let finished;
    let p = new Bluebird.Promise((resolve, reject) => {
      finished = (err, stdout, stderr) => {
        if (exited) {
          return;
        }
        exited = true;

        if (err) {
          return reject(err);
        }

        return resolve({ stdout: stdout, stderr: stderr });
      };
    });

    hookProcessPromise.then(hookProcess => {
      this.hookProcess = hookProcess;

      hookProcess.on('processExit', (code, stdout, stderr) => {
        if (code !== 0) {
          return finished(createError('Non-zero exit code: ' + code, hook, stdout, stderr));
        }

        finished(null, stdout, stderr);
      });

      hookProcess.on('processError', (err, stdout, stderr) => {
        finished(err, stdout, stderr);
      });

      if (waitForText) {
        hookProcess.onStdOut(this.varsub(waitForText), finished, waitForTextTimeout);
      }
      if (badText) {
        hookProcess.onStdOut(this.varsub(badText), (err, stdout, stderr) => {
          if (err) {
            return finished(null, stdout, stderr);
          }

          finished(createError('Found bad match (' + badText + ')', hook, stdout, stderr));
        }, badTextTimeout);
      }
    });

    return p.asCallback(callback);
  }

  varsubParams() {
    return {
      host: this.config.get('host'),
      port: this.config.get('port'),
      url: this.config.get('url')
    };
  }

  varsub(thing, data) {
    if (Array.isArray(thing)) {
      return thing.map(str => this.varsub(str, data));
    } else {
      thing = template(thing, this.varsubParams());
      thing = data ? template(thing, data) : thing;
      return thing;
    }
  }

  stop() {
    if (this.hookProcess) {
      return this.hookProcess.kill();
    }

    return Bluebird.resolve();
  }

  static with(config, hook, data) {
    let hookRunner = new HookRunner(config);

    return hookRunner.run(hook, data).catch(err => {
      log.error(hook + ' error', err);

      throw err;
    }).disposer(() => hookRunner.stop());
  }
};

function createError(message, hook, stdout, stderr) {
  return new Error(message + '\nHook: ' + hook + '\nStdout:\n' + stdout + '\nStderr:\n' + stderr);
}
