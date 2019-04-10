'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('base-cwd', 'cwd');
require('kind-of', 'typeOf');
require('is-absolute', 'isAbsolute');
require('is-valid-app', 'isValid');
require('through2', 'through');
require('mixin-deep', 'merge');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
