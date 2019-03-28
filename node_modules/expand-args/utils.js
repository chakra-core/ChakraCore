'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Module dependencies
 */

require('expand-object', 'expand');
require('kind-of', 'typeOf');
require('minimist');
require('mixin-deep', 'merge');
require('omit-empty');
require('set-value', 'set');
require = fn;

/**
 * Expose `utils`
 */

module.exports = utils;
