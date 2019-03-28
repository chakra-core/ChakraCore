'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('base-pkg', 'pkg');
require('arr-flatten', 'flatten');
require('array-unique', 'unique');
require('camel-case', 'camelcase');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('has-value');
require('has-glob');
require('kind-of', 'typeOf');
require('mixin-deep', 'merge');
require('inflection', 'inflect');
require('load-templates', 'loader');
require('matched', 'glob');
require('map-schema', 'Schema');
require('resolve');
require = fn;

/**
 * Return true if value is false, undefined, null, an empty array
 * or empty object.
 */

utils.isEmpty = function(val) {
  if (typeof val === 'function') {
    return false;
  }
  return !utils.hasValue(val);
};

/**
 * Return true if value is an object
 */

utils.isObject = function(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Cast `val` to an array.
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Expose `utils`
 */

module.exports = utils;
