'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('base-plugins', 'plugins');
require('define-property', 'define');
require('expand-target', 'Target');
require('isobject', 'isObject');
require('is-scaffold');
require('mixin-deep', 'merge');
require('scaffold', 'Scaffold');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
