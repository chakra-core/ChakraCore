'use strict';

const chalk = require('chalk');
const logger = require('heimdalljs-logger')('ember-cli:watcher');
const CoreObject = require('core-object');
const serveURL = require('../utilities/get-serve-url');

class Watcher extends CoreObject {
  constructor(_options) {
    super(_options);

    this.verbose = true;

    let options = this.buildOptions();

    logger.info('initialize %o', options);

    this.serving = _options.serving;
    this.watcher = this.watcher || this.constructWatcher(options);

    this.watcher.on('error', this.didError.bind(this));
    this.watcher.on('change', this.didChange.bind(this));
    this.serveURL = serveURL;
  }

  constructWatcher(options) {
    return new (require('ember-cli-broccoli-sane-watcher'))(this.builder, options);
  }

  didError(error) {
    logger.info('didError %o', error);
    this.ui.writeError(error);
    this.analytics.trackError({
      description: error && error.name,
    });
  }

  then() {
    return this.watcher.then.apply(this.watcher, arguments);
  }

  didChange(results) {
    logger.info('didChange %o', results);

    let totalTime = results.totalTime / 1e6;
    let message = chalk.green(`Build successful (${Math.round(totalTime)}ms)`);

    this.ui.writeLine('');

    if (this.serving) {
      message += ` – Serving on ${this.serveURL(this.options, this.options.project)}`;
    }

    this.ui.writeLine(message);

    this.analytics.track({
      name: 'ember rebuild',
      message: `broccoli rebuild time: ${totalTime}ms`,
    });

    /*
     * We use the `rebuild` category in our analytics setup for both builds
     * and rebuilds. This is a bit confusing, but the actual thing we
     * delineate on in the reports is the `variable` value below. This is
     * used both here and in `lib/tasks/build.js`.
     */
    this.analytics.trackTiming({
      category: 'rebuild',
      variable: 'rebuild time',
      label: 'broccoli rebuild time',
      value: Number(totalTime),
    });
  }

  on() {
    this.watcher.on.apply(this.watcher, arguments);
  }

  off() {
    this.watcher.off.apply(this.watcher, arguments);
  }

  buildOptions() {
    let watcher = this.options && this.options.watcher;

    if (watcher && ['polling', 'watchman', 'node', 'events'].indexOf(watcher) === -1) {
      throw new Error(`Unknown watcher type --watcher=[polling|watchman|node|events] but was: ${watcher}`);
    }

    return {
      verbose: this.verbose,
      poll: watcher === 'polling',
      watchman: watcher === 'watchman' || watcher === 'events',
      node: watcher === 'node',
    };
  }
}

module.exports = Watcher;
