/*!
 * base-argv <https://github.com/jonschlinkert/base-argv>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var debug = require('debug')('base:argv');
var utils = require('./utils');

module.exports = function(config) {
  return function() {
    if (!this.isApp || this.isRegistered('base-argv')) return;
    debug('initializing <%s>, called from <%s>', __filename, module.parent.id);

    /**
     * Process and normalize command line arguments.
     *
     * @param {Object} `argv`
     * @param {Object} `options`
     * @return {Object}
     * @api public
     */

    this.define('argv', function(argv, options) {
      if (argv.processed ===  true) {
        return argv;
      }

      debug('processing argv object', argv);

      var orig = Array.isArray(argv) ? argv.slice() : utils.extend({}, argv);
      var opts = utils.extend({}, config, this.options, options, argv);
      var args = processArgv(this, argv, opts);
      if (args.expand === 'false' || opts.expand === false) {
        delete args.expand;
        return args;
      }

      utils.define(args, 'orig', orig);
      utils.define(args, 'processed', true);

      this.emit('argv', args);
      return args;
    });
  };
};

/**
 * Expand command line arguments into the format we need to pass
 * to `app.cli.process()`.
 *
 * @param {Object} `app` Application instance
 * @param {Object} `argv` argv object, parsed by minimist
 * @param {Array} `options.first` The keys to pass to `app.cli.process()` first.
 * @param {Array} `options.last` The keys to pass to `app.cli.process()` last.
 * @param {Array} `options.keys` Flags to whitelist
 * @return {Object} Returns the `argv` object with sorted keys.
 */

function processArgv(app, argv, options) {
  var opts = utils.extend({}, options);
  if (opts.expand === false || argv.expand === 'false') {
    return argv;
  }

  argv = utils.expandArgs(argv, opts);
  return sortArgs(app, argv, opts);
}

/**
 * Sort arguments so that `app.cli.process` executes commands
 * in the order specified.
 *
 * @param {Object} `app` Application instance
 * @param {Object} `argv` The expanded argv object
 * @param {Object} `options`
 * @param {Array} `options.first` The keys to run first.
 * @param {Array} `options.last` The keys to run last.
 * @return {Object} Returns the `argv` object with sorted keys.
 */

function sortArgs(app, argv, options) {
  options = options || [];

  var first = options.first || [];
  var last = options.last || [];
  var cliKeys = [];

  if (app.cli && app.cli.keys) {
    cliKeys = app.cli.keys;
  }

  var keys = utils.union(first, cliKeys, Object.keys(argv));
  keys = utils.diff(keys, last);
  keys = utils.union(keys, last);

  var len = keys.length;
  var idx = -1;
  var res = {};

  while (++idx < len) {
    var key = keys[idx];
    if (argv.hasOwnProperty(key)) {
      res[key] = argv[key];
    }
  }
  return res;
}
