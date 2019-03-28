'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Module dependencies
 */

require('define-property', 'define');
require('get-value', 'get');
require('is-valid-app', 'isValid');
require('isobject', 'isObject');
require('mixin-deep', 'merge');
require('option-cache', 'Options');
require('set-value', 'set');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
