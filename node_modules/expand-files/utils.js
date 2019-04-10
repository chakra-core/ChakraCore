'use strict';

/**
 * Expose `utils`
 */

var utils = require('lazy-cache')(require);
var fn = require;

/**
 * Lazily required module dependencies
 */

require = utils;
require('base-plugins', 'plugins');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('glob-parent', 'parent');
require('has-glob');
require('matched', 'glob');
require('normalize-config', 'normalize');
require('object.omit', 'omit');
require('relative', 'relative');
require('resolve-dir', 'resolve');
require = fn;

/**
 * Resolve the `base` path for a filepath.
 */

utils.base = function(src, options) {
  var opts = utils.extend({}, options);
  if (opts.base) {
    return opts.base === '.' ? '' : opts.base;
  }
  var pattern = Array.isArray(src) ? src[0] : src;
  var base = utils.parent(pattern);
  if (base === '.') {
    return '';
  }
  return base;
};

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Expose `utils`
 */

module.exports = utils;
