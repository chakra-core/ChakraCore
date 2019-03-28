'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('data-store', 'Store');
require('yargs-parser', 'parser');
require('extend-shallow', 'extend');
require = fn;

/**
 * Ensure that the value passed in is an array.
 * Returns an empty array for falsey values.
 *
 * ```js
 * var arr = utils.arrayify('foo');
 * //=> ['foo']
 * ```
 * @param  {*} `val` Value to arrayify
 * @return {Array}
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Get the values from the `argv` array that should be set in the store.
 * This will decrement the index if the first value contains an `=`.
 * This ensures that when using `--macro=set` or `--macro=foo` the correct values are set.
 *
 * @param  {Array} `argv` Original argv array before parsing.
 * @param  {Number} `idx` Index where to start slicing the array.
 * @return {Array} Resulting array after slicing.
 */

utils.getValues = function(argv, idx) {
  return argv[0].indexOf('=') === -1
    ? argv.slice(idx)
    : argv.slice(idx - 1);
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
