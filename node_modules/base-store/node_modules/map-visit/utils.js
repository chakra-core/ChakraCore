'use strict';

var utils = require('lazy-cache')(require);
var fn = require;

require = utils; // trick browserify
require('object-visit', 'visit');
require = fn;

/**
 * Expose utils
 */

module.exports = utils;
