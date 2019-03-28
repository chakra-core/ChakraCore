'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Utils
 */

require('async-array-reduce', 'reduce');
require('file-is-binary', 'isBinary');
require('is-valid-app', 'isValid');
require('mixin-deep', 'merge');
require('through2', 'through');
require = fn;

utils.debug = require('debug')('base:assemble:render-file');

/**
 * Expose `utils`
 */

module.exports = utils;
