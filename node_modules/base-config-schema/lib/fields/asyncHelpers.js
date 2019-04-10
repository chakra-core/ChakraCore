'use strict';

var normalize = require('../normalize');
var debug = require('../debug');
var utils = require('../utils');

/**
 * Register async template helpers. Can be an array of module names or filepaths, or
 * an object where the keys are filepaths or module names, and the values are options
 * objects.
 *
 * ```js
 * {
 *   "asyncHelpers": ["helper-foo", "helper-bar"]
 * }
 * ```
 * @name asyncHelpers
 * @api public
 */

module.exports = function(app, options) {
  return function(val, key, config, schema) {
    if (!val) return null;
    debug.field(key, val);
    if (utils.isEmpty(val)) return;
    return normalize(val, key, config, schema);
  };
};
