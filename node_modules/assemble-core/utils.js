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

require('assemble-fs', 'fs');
require('assemble-render-file', 'render');
require('assemble-streams', 'streams');
require('base-task', 'tasks');
require('define-property', 'define');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
