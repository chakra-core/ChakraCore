'use strict';

var utils = module.exports = require('lazy-cache')(require);
var warnings = require('./warnings');
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('arr-union', 'union');
require('collection-visit', 'visit');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('get-value', 'get');
require('is-primitive');
require('kind-of', 'typeOf');
require('log-utils', 'log');
require('longest');
require('mixin-deep', 'merge');
require('object.omit', 'omit');
require('object.pick', 'pick');
require('omit-empty');
require('pad-right', 'pad');
require('set-value', 'set');
require('sort-object-arrays', 'sortArrays');
require('union-value');
require = fn;

/**
 * Logging colors and symbols
 */

utils.symbol = utils.log.symbols;
utils.color = utils.log.colors;

/**
 * Return true if `obj` has property `key`, and its value is
 * not undefind, null or an empty string.
 *
 * @param {any} `val`
 * @return {Boolean}
 */

utils.hasValue = function(key, target) {
  if (!target.hasOwnProperty(key)) {
    return false;
  }
  var val = target[key];
  if (typeof val === 'undefined' || val === 'undefined') {
    return false;
  }
  if (val === null || val === '') {
    return false;
  }
  return true;
};

utils.hasElement = function(key, target) {
  if (typeof target === 'string') {
    target = [target];
  }
  return target.indexOf(key) === -1;
};

/**
 * Return true if `val` is empty
 */

utils.isEmpty = function(val) {
  if (utils.isPrimitive(val)) {
    return val === 0 || val === null || typeof val === 'undefined';
  }
  if (Array.isArray(val)) {
    return val.length === 0;
  }
  if (utils.isObject(val)) {
    return Object.keys(utils.omitEmpty(val)).length === 0;
  }
};

/**
 * Return true if `val` is an object, not an array or function.
 *
 * @param {any} `val`
 * @return {Boolean}
 */

utils.isObject = function(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Cast `val` to an array.
 *
 * @param {String|Array} val
 * @return {Array}
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Return the indefinite article(s) to use for the given
 * JavaScript native type or types.
 *
 * @param {String|Array} `types`
 * @return {String}
 */

utils.article = function(types) {
  if (typeof types === 'string' || types.length === 1) {
    var prefix = /^[aeiou]/.test(String(types)) ? 'an ' : 'a ';
    return prefix + types;
  }
  return types.map(function(type) {
    return utils.article(type);
  }).join(' or ');
};

/**
 * Format a line in the warning table
 */

utils.formatWarning = function(warning) {
  var method = warning.method;
  var msg = utils.log.warning;
  msg += ' ';
  msg += utils.color.yellow('warning');
  msg += ' ';
  if (method === 'custom') {
    msg += warning.message;
  } else {
    msg += warnings[warning.method](warning);
  }
  return msg;
};
