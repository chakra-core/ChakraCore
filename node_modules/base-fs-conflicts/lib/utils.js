'use strict';

var block = require('set-blocking');
process.on('exit', function() {
  block(true);
});

var utils = require('lazy-cache')(require);
var fn = require;
require = utils; // eslint-disable-line

/**
 * Lazily required module dependencies
 */

require('log-utils', 'log');
require('base-cwd', 'cwd');
require('bytes');
require('cli-table', 'Table');
require('dateformat-light', 'dateFormat');
require('detect-conflict', 'detect');
require('diff');
require('extend-shallow', 'extend');
require('inquirer2', 'inquirer');
require('istextorbinary', 'itob');
require('read-chunk');
require('through2', 'through');
require = fn; // eslint-disable-line

/**
 * Utils
 */

utils.symbol = utils.log.symbol;
utils.colors = utils.log.colors;

/**
 * format a relative path
 */

utils.relative = function(file) {
  return utils.colors.yellow(file.relative);
};

/**
 * Log symbols
 */

var info = utils.colors.cyan(utils.symbol.info);
var success = utils.colors.green(utils.symbol.success);
var warning = utils.colors.yellow(utils.symbol.warning);
var error = utils.colors.red(utils.symbol.error);

/**
 * Log out possible actions when a conflict is found
 */

utils.action = {
  identical: function noop() {
    // console.log('%s existing file is identical, skipping %s', info, utils.relative(file));
  },
  yes: function(file) {
    console.log('%s overwriting %s', success, utils.relative(file));
  },
  no: function(file) {
    console.log('%s skipping %s', warning, utils.relative(file));
  },
  all: function(file) {
    console.log('%s all remaining files will be written, and will overwrite any existing files.', success);
  },
  abort: function(file) {
    console.log('%s stopping, no files will be written by this task', error);
  },
  diff: function(file) {
    console.log('%s diff comparison of %s and existing content', info, utils.relative(file));
  }
};

/**
 * Expose `utils`
 */

module.exports = utils;
