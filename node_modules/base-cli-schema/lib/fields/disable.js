'use strict';

var normalize = require('../normalize');

/**
 * Disable a configuration setting. This is the API-equivalent of
 * calling `app.disable('foo')`, or `app.option('foo', false)`.
 *
 * ```sh
 * $ --disable=foo
 * # {options: {foo: false}}
 * ```
 * @name disable
 * @api public
 * @cli public
 */

module.exports = function(app, base, options) {
  return normalize(app, base, options);
};
