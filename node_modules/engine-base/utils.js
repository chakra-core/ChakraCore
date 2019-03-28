'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('delimiter-regex', 'delims');
require('engine', 'Engine');
require('mixin-deep', 'merge');
require('object.omit', 'omit');
require('object.pick', 'pick');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
