'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('base-data', 'data');
require('camel-case', 'camelcase');
require('clone-deep', 'clone');
require('is-valid-app', 'isValid');
require('expand-pkg', 'Expand');
require('namify');
require('repo-utils', 'repo');
require = fn;

/**
 * Returns true if `str` is a string with length greater than zero
 */

utils.isString = function(val) {
  return val && typeof val === 'string';
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
