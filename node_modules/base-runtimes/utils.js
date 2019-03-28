'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('extend-shallow', 'extend');
require('is-valid-app', 'isValid');
require('log-utils', 'log');
require('micromatch', 'mm');
require('time-diff', 'Time');
require = fn;

/**
 * Expose `colors` from log-utils
 */

utils.colors = utils.log.colors;

/**
 * Formatted timestamp
 */

utils.timestamp = function() {
  var args = [].slice.call(arguments);
  args.unshift(utils.log.timestamp);
  console.error.apply(console, args);
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
