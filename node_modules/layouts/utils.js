'use strict';

/**
 * Lazily required module dependencies
 */

var lazy = require('lazy-cache')(require);
var fn = require;

require = lazy;
require('falsey', 'isFalsey');
require('delimiter-regex', 'delims');
require('get-view');
require = fn;

/**
 * Expose `utils`
 */

module.exports = lazy;
