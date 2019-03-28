'use strict';

var paths = require('../shared/paths');

/**
 * Normalize the given `--cwd` value to be an absolute filepath.
 *
 * ```sh
 * $ --cwd=foo
 * ```
 * @name cwd
 * @api public
 */

module.exports = function(app, base, options) {
  return function(val, key, config, schema) {
    return paths('cwd', val, config, key);
  };
};
