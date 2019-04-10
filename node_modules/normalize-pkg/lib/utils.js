'use strict';

var fs = require('fs');
var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module depedencies
 */

require('arr-union', 'union');
require('array-unique', 'unique');
require('extend-shallow', 'extend');
require('fs-exists-sync', 'exists');
require('get-value', 'get');
require('kind-of', 'typeOf');
require('mixin-deep', 'merge');
require('omit-empty');
require('parse-git-config');
require('repo-utils', 'repo');
require('semver');
require('stringify-author', 'stringify');
require = fn;

/**
 * Return true if `val` is empty
 */

utils.isEmpty = function(val) {
  return Object.keys(utils.omitEmpty(val)).length === 0;
};

/**
 * Return true if `val` is an object
 */

utils.isObject = function(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Return true if `val` is a string with a non-zero length
 */

utils.isString = function(val) {
  return val && typeof val === 'string';
};

/**
 * Cast `val` to an array
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Try to require the package.json at the given filepath
 */

utils.requirePackage = function(filepath) {
  filepath = path.resolve(filepath);
  try {
    var stat = fs.statSync(filepath);
    if (stat.isDirectory()) {
      filepath = path.join(filepath, 'package.json');
    } else if (!/package\.json$/.test(filepath)) {
      filepath = path.join(path.dirname(filepath), 'package.json');
    }
    return require(filepath);
  } catch (err) {};
  return {};
};

/**
 * Strip a trailing slash from a string.
 *
 * @param {String} `str`
 * @return {String}
 */

utils.stripSlash = function stripSlash(str) {
  return str.replace(/\W+$/, '');
};

/**
 * Expose `utils`
 */

module.exports = utils;
