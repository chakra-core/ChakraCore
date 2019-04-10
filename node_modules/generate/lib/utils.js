'use strict';

var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils; // eslint-disable-line

/**
 * Utils
 */

require('ask-when');
require('common-config', 'common');
require('data-store', 'Store');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('get-value', 'get');
require('global-modules', 'gm');
require('helper-ask', 'ask');
require('isobject', 'isObject');
require('log-utils', 'log');
require('macro-store', 'macros');
require('resolve-file', 'resolve');
require('strip-color', 'strip');
require('text-table', 'table');
require('through2', 'through');
require('minimist', 'parse');
require = fn;

/**
 * argv options
 */

utils.opts = {
  boolean: ['diff'],
  alias: {
    add: 'a',
    config: 'c',
    configfile: 'f',
    dest: 'd',
    diff: 'diffOnly',
    global: 'g',
    help: 'h',
    init: 'i',
    silent: 'S',
    verbose: 'v',
    version: 'V',
    remove: 'r'
  }
};

/**
 * Expose parsed `argv` and `args`
 */

utils.blacklist = [
  'generate-foo',
  'generate-bar',
  'generate-function',
  'generate-object-property'
];

utils.parseArgs = function(argv) {
  var parse = utils.macros('generate-macro-store');
  var obj = parse(argv, utils.opts);

  for (var key in obj) {
    var val = obj[key];
    if (/^no\w/.test(key)) {
      obj[key.slice(2)] = !val;
    }
  }

  if (obj.init) {
    obj._.push('init');
    delete obj.init;
  }
  if (obj.help) {
    obj._.push('help');
    delete obj.help;
  }
  return obj;
};

utils.args = process.argv.slice(2);
utils._argv = utils.parseArgs(utils.args);
utils.argv = {};

for (var key in utils._argv) {
  if (utils._argv.hasOwnProperty(key)) {
    utils.argv[key] = utils._argv[key];
  }
}

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

utils.renameKey = function(app) {
  var fn = app.options.renameKey;
  if (typeof fn !== 'function') {
    fn = function(key, file) {
      return file ? file.basename : path.basename(key);
    };
  }
  return fn;
};

utils.getTasks = function(configFile, arrays) {
  arrays = utils.arrayify(arrays);
  var tasks = [];

  if (configFile) {
    tasks = utils.arrayify(arrays[0]);
    return tasks.length >= 1 ? tasks : ['default'];
  }

  for (var i = 0; i < arrays.length; i++) {
    var arr = utils.arrayify(arrays[i]);
    // if `default` task is defined, continue
    if (arr.length === 1 && arr[0] === 'default') {
      continue;
    }
    // if nothing is defined, continue
    if (arr.length === 0) {
      continue;
    }
    tasks = arr;
    break;
  }
  return tasks;
};

utils.firstIndex = function(arr, items) {
  items = utils.arrayify(items);
  var idx = -1;
  for (var i = 0; i < arr.length; i++) {
    if (items.indexOf(arr[i]) !== -1) {
      idx = i;
      break;
    }
  }
  return idx;
};

/**
 * Return true if the generate CLI is enabled
 */

utils.runnerEnabled = function(app) {
  if (app.options.cli === true || process.env.GENERATE_CLI === 'true') {
    return true;
  }
  // if run from the generate cwd (and not using the `gen` command)
  if (process.cwd() === path.resolve(__dirname, '..')) {
    return false;
  }
  return true;
};

/**
 * Lazily expose a property on `app.cache.data`
 */

utils.lazyData = function(app, key, fn) {
  var cached;
  Object.defineProperty(app.cache.data, key, {
    set: function(val) {
      cached = val;
    },
    get: function() {
      return cached || (cached = fn.call(this));
    }
  });
};

/**
 * Expose utils
 */

module.exports = utils;
