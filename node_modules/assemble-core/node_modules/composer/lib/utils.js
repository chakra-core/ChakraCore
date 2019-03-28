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
require = utils; // eslint-disable-line

/**
 * Lazily required module dependencies
 */

require('array-unique', 'unique');
require('bach');
require('co');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('isobject');
require('is-generator');
require('is-glob');
require('micromatch', 'mm');
require('nanoseconds', 'nano');
require('pretty-time', 'time');

/**
 * Restore `require`
 */

require = fn; // eslint-disable-line

/**
 * Arrayify the value to ensure a valid array is returned.
 *
 * ```js
 * var arr = utils.arrayify('foo');
 * //=> ['foo']
 * ```
 * @param  {Mixed} `val` value to arrayify
 * @return {Array} Always an array.
 */

utils.arrayify = function(val) {
  if (!val) return [];
  return Array.isArray(val) ? val : [val];
};

/**
 * Format a task error message to include the task name at the beginning.
 *
 * ```js
 * err = utils.formatError(err);
 * //=> 'Error: in task "foo": ...'
 * ```
 *
 * @param  {Error} `err` Error object with a `.task` property
 * @return {Error} Error object with formatted error message.
 */

utils.formatError = function(err) {
  if (!err || typeof err.task === 'undefined') {
    return err;
  }
  err.message = 'in task "' + err.task.name + '": ' + err.message;
  return err;
};

/**
 * Determine if the args array has an options object at the beginning.
 *
 * ```js
 * console.log(hasOptions([{}]));
 * //=> true
 * console.log(hasOptions([]));
 * //=> false
 * ```
 * @param  {Array} `args` Array of arguments
 * @return {Boolean} true if first element is an object
 */

utils.hasOptions = function(args) {
  return args.length && utils.isobject(args[0]);
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
