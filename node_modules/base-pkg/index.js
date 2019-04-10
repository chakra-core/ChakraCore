/*!
 * base-pkg <https://github.com/node-base/base-pkg>
 *
 * Copyright (c) 2016-2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var util = require('util');
var debug = require('debug')('base:base-pkg');
var define = require('define-property');
var extend = require('extend-shallow');
var isValid = require('is-valid-app');
var pkgStore = require('pkg-store');
var Cache = require('cache-base');
var Pkg = require('expand-pkg');
var log = require('log-utils');

module.exports = function(config, fn) {
  if (typeof config === 'function') {
    fn = config;
    config = {};
  }

  return function plugin(app) {
    if (!isValid(app, 'base-pkg')) return;
    debug('initializing from <%s>', __filename);

    var pkg;
    this.define('pkg', {
      configurable: true,
      enumerable: true,
      set: function(val) {
        pkg = val;
      },
      get: function() {
        if (pkg) {
          decorate(app, pkg);
          return pkg;
        }
        var cwd = app.cwd || process.cwd();
        var opts = extend({cwd: cwd}, config, app.options);
        pkg = pkgStore(opts);
        decorate(app, pkg);
        return pkg;
      }
    });

    return plugin;
  };
};

/**
 * Utils
 */

function decorate(app, pkg) {
  if (pkg.logValue) return;
  define(pkg, 'expand', function() {
    var config = new Pkg();
    var data = extend({}, pkg.data);
    var expanded = config.expand(data);
    var cache = new Cache(expanded);
    return cache;
  });
  define(pkg, 'logValue', function(msg, val) {
    console.log(log.timestamp, msg, util.inspect(val, null, 10));
  });
  define(pkg, 'logInfo', function(msg, val) {
    val = log.colors.cyan(util.inspect(val, null, 10));
    console.log(log.timestamp, msg, val);
  });
  define(pkg, 'logWarning', function(msg, val) {
    val = log.colors.yellow(util.inspect(val, null, 10));
    console.log(log.timestamp, msg, val);
  });
  define(pkg, 'logError', function(msg, val) {
    val = log.colors.red(util.inspect(val, null, 10));
    console.log(log.timestamp, msg, val);
  });
  define(pkg, 'logSuccess', function(msg, val) {
    val = log.colors.green(util.inspect(val, null, 10));
    console.log(log.timestamp, msg, val);
  });
}
