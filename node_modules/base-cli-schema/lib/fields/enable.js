'use strict';

var normalize = require('../normalize');

/**
 * Enable a configuration setting. This is the API-equivalent of
 * calling `app.enable('foo')`, or `app.option('foo', true)`.
 *
 * ```sh
 * $ --enable=foo
 * # {options: {foo: true}}
 * ```
 * @name enable
 * @api public
 * @cli public
 */

module.exports = function(app, base, options) {
  return normalize(app, base, options);
};
