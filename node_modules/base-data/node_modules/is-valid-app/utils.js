'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('debug');
require('is-registered');
require('is-valid-instance', 'isValid');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
