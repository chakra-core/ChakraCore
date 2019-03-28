'use strict';

var fns = require('../fns');

/**
 * Load and register async template helpers from a glob or filepath.
 *
 * ```sh
 * $ app --asyncHelpers="foo.js"
 * ```
 * @name asyncHelpers
 * @api public
 */

module.exports = function(app) {
  return fns(app, 'asyncHelpers');
};
