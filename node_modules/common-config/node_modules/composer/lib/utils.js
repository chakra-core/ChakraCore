'use strict';

/**
 * Module dependencies
 */

var utils = require('lazy-cache')(require);

/**
 * Temporarily re-assign `require` to trick browserify and
 * webpack into reconizing lazy dependencies.
 *
 * This tiny bit of ugliness has the huge dual advantage of
 * only loading modules that are actually called at some
 * point in the lifecycle of the application, whilst also
 * allowing browserify and webpack to find modules that
 * are depended on but never actually called.
 */

var fn = require;
require = utils; // eslint-disable-line

/**
 * Lazily required module dependencies
 */

require('array-unique', 'unique');
require('bach');
require('co');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('isobject');
require('is-generator');
require('is-glob');
require('micromatch', 'mm');
require('nanoseconds', 'nano');

/**
 * Restore `require`
 */

require = fn; // eslint-disable-line

utils.arrayify = function(val) {
  if (!val) return [];
  return Array.isArray(val) ? val : [val];
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
