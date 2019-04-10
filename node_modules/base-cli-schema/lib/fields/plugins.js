'use strict';

var base = require('../shared/base');

/**
 * Load plugins from a filepath or glob pattern.
 *
 * ```sh
 * $ --plugins="./foo.js"
 * ```
 * @name plugins
 * @api public
 */

module.exports = function(app) {
  var field = base(app);

  return function(val, key, config, schema) {
    return field.normalize(val, key, config, schema);
  };
};
