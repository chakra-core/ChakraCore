'use strict';

var base = require('../shared/base');

/**
 * Register async template helpers at the given filepath.
 *
 * ```sh
 * $ --helpers="./foo.js"
 * ```
 * @name helpers
 * @api public
 * @cli public
 */

module.exports = function(app) {
  var field = base(app);

  return function(val, key, config, schema) {
    return field.normalize(val, key, config, schema);
  };
};
