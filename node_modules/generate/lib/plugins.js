'use strict';

var path = require('path');
var utils = require('./utils');
var plugins = require('lazy-cache')(require);
var fn = require;
require = plugins; // eslint-disable-line
var stores = {};

/**
 * Plugins
 */

require('assemble-loader', 'loader');
require('base-fs-conflicts', 'conflicts');
require('base-fs-rename', 'rename');
require('base-generators', 'generators');
require('base-npm', 'npm');
require('base-npm-prompt', 'prompt');
require('base-pipeline', 'pipeline');
require('base-questions', 'questions');
require('base-runtimes', 'runtimes');
require('base-runner', 'runner');
require('base-store', 'store');
require = fn; // eslint-disable-line

/**
 * Add logging methods
 */

plugins.logger = function(options) {
  return function() {

    function logger(prop, color) {
      color = color || 'dim';
      return function(msg) {
        var rest = [].slice.call(arguments, 1);
        return console.log
          .bind(console, utils.log.timestamp + (prop ? (' ' + utils.log[prop]) : ''))
          .apply(console, [utils.log[color](msg)].concat(rest));
      };
    };

    Object.defineProperty(this, 'log', {
      configurable: true,
      get: function() {
        function log() {
          return console.log.apply(console, arguments);
        }
        log.path = function(msg) {
          return logger(null, 'dim').apply(null, arguments);
        };
        log.time = function(msg) {
          return logger(null, 'dim').apply(null, arguments);
        };
        log.warn = function(msg) {
          return logger('warning').apply(null, arguments);
        };
        log.warning = function(msg) {
          return logger('warning', 'yellow').apply(null, arguments);
        };
        log.success = function() {
          return logger('success', 'green').apply(null, arguments);
        };
        log.info = function() {
          return logger('info', 'cyan').apply(null, arguments);
        };
        log.error = function() {
          return logger('error', 'red').apply(null, arguments);
        };
        log.__proto__ = utils.log;
        return log;
      }
    });
  };
};

/**
 * Initialize stores
 */

plugins.stores = function(proto) {
  // create `macros` store
  Object.defineProperty(proto, 'macros', {
    configurable: true,
    set: function(val) {
      stores.macros = val;
    },
    get: function() {
      return stores.macros || (stores.macros = new utils.MacroStore({name: 'generate-macros'}));
    }
  });

  // create `app.globals` store
  Object.defineProperty(proto, 'globals', {
    configurable: true,
    set: function(val) {
      stores.globals = val;
    },
    get: function() {
      return stores.globals || (stores.globals = new utils.Store('generate-globals'));
    }
  });
};

/**
 * Results the dest path to use
 */

plugins.destPath = function(options) {
  return function() {
    Object.defineProperty(this, 'destBase', {
      configurable: true,
      enumerable: true,
      set: function(dest) {
        this.cache.dest = path.resolve(dest);
      },
      get: function() {
        if (typeof this.cache.dest === 'string') {
          return path.resolve(this.cache.dest);
        }
        if (typeof this.options.dest === 'string') {
          return path.resolve(this.options.dest);
        }
        return process.cwd();
      }
    });
  };
};

plugins.dest = function(options) {
  return function(app) {
    var dest = this.dest;

    this.define('dest', function(dir, options) {
      var opts = utils.extend({ cwd: this.cwd }, options);
      if (typeof dir !== 'function' && typeof this.rename === 'function') {
        dir = this.rename(dir);
      }
      return dest.call(this, dir, opts);
    });
  };
};

plugins.src = function(collection) {
  return function(app) {
    collection = collection || 'templates';
    var src = this.src;

    this.define('src', function(patterns, options) {
      var opts = utils.extend({}, options);
      if (typeof opts.collection === 'undefined') {
        opts.collection = collection;
      }
      return src.call(this, patterns, opts);
    });
  };
};

/**
 * Expose plugins
 */

module.exports = plugins;
