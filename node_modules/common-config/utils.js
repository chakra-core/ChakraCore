'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('composer', 'Composer');
require('data-store', 'Store');
require('get-value', 'get');
require('log-utils', 'log');
require('object.pick', 'pick');
require('omit-empty');
require('question-cache', 'Questions');
require('set-value', 'set');
require('strip-color');
require('tableize-object', 'tableize');
require('text-table', 'table');
require('yargs-parser', 'yargs');
require = fn;

/**
 * Expose `utils` modules
 */

module.exports = utils;
