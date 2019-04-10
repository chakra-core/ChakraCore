'use strict';

const TestTask = require('./test');
const Promise = require('rsvp').Promise;
const chalk = require('chalk');
const SilentError = require('silent-error');

class TestServerTask extends TestTask {
  invokeTestem(options) {
    let task = this;

    return new Promise((resolve, reject) => {
      task.testem.setDefaultOptions(task.defaultOptions(options));
      task.testem.startDev(task.transformOptions(options), (exitCode, error) => {
        if (error) {
          reject(error);
        } else if (exitCode !== 0) {
          reject(new SilentError('Testem finished with non-zero exit code. Tests failed.'));
        } else {
          resolve(exitCode);
        }
      });
    });
  }

  run(options) {
    let ui = this.ui;
    let testem = this.testem;
    let task = this;

    // The building has actually started already, but we want some output while we wait for the server

    return new Promise((resolve, reject) => {
      if (options.path) {
        resolve(task.invokeTestem(options));
        return;
      }

      ui.startProgress(chalk.green('Building'), chalk.green('.'));

      let watcher = options.watcher;
      let started = false;

      // Wait for a build and then either start or restart testem
      watcher.on('change', () => {
        try {
          if (started) {
            testem.restart();
          } else {
            started = true;

            ui.stopProgress();
            resolve(task.invokeTestem(options));
          }
        } catch (e) {
          reject(e);
        }
      });
    });
  }

  /**
   * Exit silently
   *
   * @private
   * @method onInterrupt
   */
  onInterrupt() {
    // We don't have any hanging promise to resolve here because the SIGINT is
    // captured and handled by testem
  }
}

module.exports = TestServerTask;
