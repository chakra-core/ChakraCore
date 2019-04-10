'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('async-each-series', 'eachSeries');
require('data-store', 'DataStore');
require('extend-shallow', 'extend');
require('fs-exists-sync', 'exists');
require('get-value', 'get');
require('is-valid-app', 'isValid');
require('isobject', 'isObject');
require = fn;

/**
 * Cast `val` to an array
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Remove names that are already listed in `package.json`
 */

utils.unique = function(pkg, type, keys) {
  var names = utils.arrayify(Object.keys(utils.get(pkg, type) || {}));
  return utils.arrayify(keys).filter(function(name) {
    return names.indexOf(name) === -1;
  });
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
