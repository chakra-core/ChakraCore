'use strict';

/**
 * Module dependencies
 */

var utils = require('lazy-cache')(require);

/**
 * Temporarily re-assign `require` to trick browserify and
 * webpack into reconizing lazy dependencies.
 *
 * This tiny bit of ugliness has the huge dual advantage of
 * only loading modules that are actually called at some
 * point in the lifecycle of the application, whilst also
 * allowing browserify and webpack to find modules that
 * are depended on but never actually called.
 */

var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('noncharacters', 'nonchars');
require('is-extendable', 'isObject');
require('arr-flatten', 'flatten');

/**
 * Restore `require`
 */

require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
