'use strict';

/**
 * Module dependencies
 */

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('base-plugins', 'plugins');
require('define-property', 'define');
require('expand-files', 'Expand');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
