'use strict';

var fns = require('../fns');

/**
 * Load engines from a filepath or glob pattern.
 *
 * ```sh
 * $ app --engines="./foo.js"
 * ```
 * @name engines
 * @api public
 */

module.exports = function(app) {
  return fns(app, 'engines');
};
