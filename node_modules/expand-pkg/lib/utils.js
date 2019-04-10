'use strict';

var fs = require('fs');
var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module depedencies
 */

require('defaults-deep', 'defaults');
require('get-value', 'get');
require('kind-of', 'typeOf');
require('load-pkg');
require('mixin-deep', 'merge');
require('omit-empty');
require('normalize-pkg', 'Pkg');
require('parse-author');
require('parse-git-config');
require('repo-utils', 'repo');
require = fn;

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
 * Expose `utils`
 */

module.exports = utils;
