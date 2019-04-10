'use strict';

/**
 * Lazily required module dependencies
 */

var utils = require('lazy-cache')(require);
var fn = require;

require = utils; // trick browserify so we can lazy-cache
require('map-visit');
require('object-visit', 'visit');
require = fn;

/**
 * Expose `utils`
 */

module.exports = utils;
