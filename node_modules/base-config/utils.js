'use strict';

var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('isobject', 'isObject');
require('map-config', 'mapper');
require('resolve-dir', 'resolve');
require = fn;

/**
 * Try to require a module, fail silently if not found.
 */

utils.tryRequire = function(name, cwd) {
  try {
    return require(name);
  } catch (err) {};
  return require(path.resolve(cwd || process.cwd(), name));
};

/**
 * Cast the given value to an array
 */

utils.arrayify = function(val) {
  if (!val) return [];
  if (typeof val === 'string') {
    return val.split(',');
  }
  return Array.isArray(val) ? val : [val];
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
