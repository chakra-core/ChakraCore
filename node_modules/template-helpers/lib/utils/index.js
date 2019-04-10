'use strict';

/**
 * Module dependencies
 */

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Utils
 */

require('any');
require('arr-union', 'union');
require('arr-flatten', 'flatten');
require('center-align', 'alignCenter');
require('get-value', 'get');
require('is-absolute');
require('is-number');
require('is-plain-object');
require('kind-of', 'typeOf');
require('make-iterator');
require('object.omit', 'omit');
require('relative');
require('right-align', 'alignRight');
require('strip-color');
require('titlecase');
require('to-gfm-code-block', 'block');
require('word-wrap', 'wrap');
require = fn;

utils.isEmpty = function(val) {
  if (typeof val === 'function') {
    return false;
  }
  if (utils.isObject(val)) {
    val = Object.keys(val);
  }
  if (Array.isArray(val)) {
    return val.length === 0;
  }
  if (typeof val === 'undefined') {
    return true;
  }
  if (val === 0) {
    return false;
  }
  if (val == null) {
    return true;
  }
};

/**
 * Get the "title" from a markdown link
 */

utils.getTitle = function(str) {
if (/^\[[^\]]+\]\(/.test(str)) {
    var m = /^\[([^\]]+)\]/.exec(str);
    if (m) return m[1];
  }
  return str;
};

/**
 * Returns true if the given `val` is an object.
 *
 * ```js
 * utils.isObject('foo');
 * //=> false
 *
 * utils.isObject({});
 * //=> true
 * ```
 * @param {any} `val`
 * @return {Boolean}
 * @api public
 */

utils.isObject = function(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Stringify HTML tag attributes from the given `object`.
 *
 * @param {Object} `object` Object of attributes as key-value pairs.
 * @return {String} Stringified attributes, e.g. `foo="bar"`
 * @api public
 */

utils.toAttributes = function(obj) {
  return Object.keys(obj).map(function(key) {
    return key + '="' + obj[key] + '"';
  }).join(' ');
};

/**
 * Generate a random number
 *
 * @param  {Number} `min`
 * @param  {Number} `max`
 * @return {Number}
 */

utils.random = function(min, max) {
  return min + Math.floor(Math.random() * (max - min + 1));
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
