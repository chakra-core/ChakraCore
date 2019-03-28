'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Utils
 */

require('base-option', 'option');
require('extend-shallow', 'extend');
require('is-valid-app', 'isValid');
require('kind-of', 'typeOf');
require('stream-combiner', 'combine');
require('through2', 'through');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
