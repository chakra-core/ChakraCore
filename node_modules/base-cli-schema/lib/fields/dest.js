'use strict';

var paths = require('../shared/paths');

/**
 * Normalize the given `--dest` path. If defined by the user,
 * `dest` is normalized to be an absolute filepath, otherwise
 * `dest` is set to `app.cwd`.
 *
 * ```sh
 * $ --dest=foo
 * ```
 * @name dest
 * @api public
 */

module.exports = function(app, base, options) {
  return function(val, key, config, schema) {
    val = paths('dest', val, config, schema);
    if (typeof val === 'undefined') {
      return;
    }
    val = config[key] = app.cwd;
    return val;
  };
};
