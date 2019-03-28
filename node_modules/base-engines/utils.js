'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('engine-cache', 'Engines');
require('define-property', 'define');
require('is-valid-app', 'isValid');
require = fn;

/**
 * Arrayify the given value by casting it to an array.
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Return true if the given value is a string.
 */

utils.isString = function(val) {
  return val && typeof val === 'string';
};

/**
 * Ensure file extensions are formatted properly for lookups.
 *
 * ```js
 * utils.formatExt('hbs');
 * //=> '.hbs'
 *
 * utils.formatExt('.hbs');
 * //=> '.hbs'
 * ```
 *
 * @param {String} `ext` File extension
 * @return {String}
 * @api public
 */

utils.formatExt = function formatExt(ext) {
  if (typeof ext !== 'string') {
    throw new Error('expected a string');
  }
  if (ext.charAt(0) !== '.') {
    return '.' + ext;
  }
  return ext;
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
