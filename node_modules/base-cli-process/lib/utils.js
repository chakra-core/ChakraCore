'use strict';

var os = require('os');
var path = require('path');
var util = require('util');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

// plugins
require('base-cli', 'cli');
require('base-cli-schema', 'schema');
require('base-cwd', 'cwd');
require('base-config-process', 'config');
require('base-option', 'option');
require('base-pkg', 'pkg');

// utils
require('arrayify-compact', 'arrayify');
require('arr-union');
require('fs-exists-sync', 'exists');
require('is-valid-app', 'isValid');
require('kind-of', 'typeOf');
require('log-utils', 'log');
require('mixin-deep', 'merge');
require('object.pick', 'pick');
require('union-value', 'union');
require = fn;

utils.colors = utils.log.colors;
utils.magenta = utils.colors.magenta;
utils.cyan = utils.colors.cyan;

/**
 * Ensure the requisite plugins are registered when a command is given,
 * otherwise throw an error.
 */

utils.assertPlugins = function(app, key) {
  switch(key) {
    case 'pkg':
    case 'config':
      app.assertPlugin('base-pkg');
      break;
    case 'option':
    case 'options':
      app.assertPlugin('base-option');
      break;
    case 'task':
    case 'tasks':
      app.assertPlugin('base-task');
      break;
    case 'helper':
    case 'helpers':
    case 'asyncHelper':
    case 'asyncHelpers':
    case 'engine':
    case 'engines':
      app.assertPlugin('templates');
      break;
    case 'plugin':
    case 'plugins':
      app.assertPlugin('base-pipeline');
      break;
    case 'globals':
    case 'store':
      app.assertPlugin('base-store');
      break;
    case 'data': {
      app.assertPlugin('base-data:cache.data');
      break;
    }
  }
};

/**
 * Format a value to be displayed in the command line
 */

utils.formatValue = function(val) {
  return utils.cyan(util.inspect(val, null, 10));
};

/**
 * Get a home-relative filepath
 */

utils.homeRelative = function(filepath) {
  filepath = path.relative(os.homedir(), path.resolve(filepath));
  return path.normalize(filepath);
};

/**
 * Log out a formatted timestamp
 */

utils.timestamp = function() {
  var args = [].slice.call(arguments);
  args.unshift(utils.log.timestamp);
  console.log.apply(console, args);
};

/**
 * Strip the quotes from a string
 */

utils.stripQuotes = function(str) {
  return str.replace(/^['"]|['"]$/g, '');
};

/**
 * Returns true if `val` is true or is an object with `show: true`
 *
 * @param {String} `val`
 * @return {Boolean}
 */

utils.show = function(val) {
  return utils.isObject(val) && val.show === true;
};

/**
 * Return true if a value is an object.
 */

utils.isObject = function(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Return true if a value is a string with non-zero length.
 */

utils.isString = function(val) {
  return val && typeof val === 'string';
};

/**
 * Arrayify, flatten and uniquify `arr`
 */

utils.unionify = function(arr) {
  return utils.arrayify(utils.arrUnion.apply(null, arguments));
};

/**
 * Expose `utils`
 */

module.exports = utils;
