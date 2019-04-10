'use strict';

var base = require('../shared/base');

/**
 * Load engines from a filepath or glob pattern.
 *
 * ```sh
 * $ --engines="./foo.js"
 * ```
 * @name engines
 * @api public
 */

module.exports = function(app) {
  var field = base(app);

  return function(val, key, config, schema) {
    return field.normalize(val, key, config, schema);
  };
};
