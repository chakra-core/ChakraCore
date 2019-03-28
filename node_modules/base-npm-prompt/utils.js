'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('async-each', 'each');
require('base-questions', 'questions');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('is-valid-app', 'isValid');
require('log-utils', 'log');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
