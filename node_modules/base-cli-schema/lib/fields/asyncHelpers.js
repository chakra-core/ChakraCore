'use strict';

var helpers = require('./helpers');

/**
 * Register async template helpers at the given filepath.
 *
 * ```sh
 * $ --asyncHelpers="./foo.js"
 * ```
 * @name asyncHelpers
 * @api public
 * @cli public
 */

module.exports = function(app) {
  return helpers(app);
};
