'use strict';

var fs = require('fs');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('cwd');
require('expand-tilde');
require('extend-shallow', 'extend');
require('fs-exists-sync', 'exists');
require('homedir-polyfill', 'home');
require('resolve');
require = fn;

/**
 * Return the given `val`
 */

utils.identity = function(val) {
  return val;
};

/**
 * Decorate `file` with `stat`
 */

utils.decorate = function(file, fn) {
  var resolve = fn || utils.identity;
  var stat;

  Object.defineProperty(file, 'stat', {
    configurable: true,
    set: function(val) {
      stat = val;
    },
    get: function() {
      if (stat) {
        return stat;
      }
      if (!this.path) {
        throw new Error('expected file.path to be a string, cannot get file.stat');
      }
      if (utils.exists(this.path)) {
        stat = fs.lstatSync(this.path);
        return stat;
      }
      return null;
    }
  });

  // support a custom `resolve` fn
  var result = resolve.call(file, file);
  if (typeof result === 'string') {
    file.path = result;
  } else if (result) {
    file = result;
  }

  return file;
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
