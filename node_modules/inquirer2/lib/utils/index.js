'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('is-plain-object');
require('array-unique', 'unique');
require('extend-shallow', 'extend');
require('arr-flatten', 'flatten');
require('arr-pluck', 'pluck');
require('is-number');
require('ansi-escapes');
require('ansi-regex');
require('chalk');
require('cli-cursor');
require('cli-width');
require('figures');
require('readline2', 'readlineFacade');
require('run-async');
require('rx-lite', 'rx');
require('strip-color');
require('through2');
require('lodash.where', 'where');
require = fn;

/**
 * Noop
 */

utils.noop = function() {}

/**
 * Expose `utils` modules
 */

module.exports = utils;
