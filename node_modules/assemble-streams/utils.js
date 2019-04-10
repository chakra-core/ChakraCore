'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('assemble-handle', 'handle');
require('define-property', 'define');
require('is-valid-app', 'isValid');
require('kind-of', 'typeOf');
require('match-file', 'match');
require('src-stream', 'src');
require('through2', 'through');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
