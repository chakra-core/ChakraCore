'use strict';

/**
 * Disable one or more options. This is the API-equivalent of
 * calling `app.disable('foo')`, or `app.option('foo', false)`.
 *
 * ```js
 * {disable: 'foo'}
 * // or
 * {disable: ['foo', 'bar']}
 * ```
 * @name disable
 * @api public
 */

module.exports = function(app, base, options) {
  return function(val, key, config, schema) {
    if (typeof val === 'string') {
      val = [val];
    }
    if (Array.isArray(val)) {
      val.forEach(function(key) {
        app.disable(key);
      });
    }
    return val;
  };
};
