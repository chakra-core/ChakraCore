'use strict';

const chalk = require('chalk');
const Task = require('../models/task');
const Watcher = require('../models/watcher');
const Builder = require('../models/builder');
const RSVP = require('rsvp');

class BuildWatchTask extends Task {
  constructor(options) {
    super(options);

    this._builder = null;
    this._runDeferred = null;
  }

  run(options) {
    let { ui } = this;

    ui.startProgress(
      chalk.green('Building'), chalk.green('.')
    );

    this._runDeferred = RSVP.defer();

    let builder = this._builder = options._builder || new Builder({
      ui,
      outputPath: options.outputPath,
      environment: options.environment,
      project: this.project,
    });

    ui.writeLine(`Environment: ${options.environment}`);

    let watcher = options._watcher || new Watcher({
      ui,
      builder,
      analytics: this.analytics,
      options,
    });

    return watcher.then(() => this._runDeferred.promise /* Run until failure or signal to exit */);
  }

  /**
   * Exit silently
   *
   * @private
   * @method onInterrupt
   */
  onInterrupt() {
    return this._builder.cleanup().then(() => this._runDeferred.resolve());
  }
}

module.exports = BuildWatchTask;
