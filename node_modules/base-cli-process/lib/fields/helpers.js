'use strict';

var fns = require('../fns');

/**
 * Load and register async template helpers from a glob or filepath.
 *
 * ```sh
 * $ app --helpers="foo.js"
 * ```
 * @name helpers
 * @api public
 */

module.exports = function(app) {
  return fns(app, 'helpers');
};
