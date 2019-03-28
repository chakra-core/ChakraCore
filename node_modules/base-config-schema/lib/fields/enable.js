'use strict';

/**
 * Enable one or more options. This is the API-equivalent of
 * calling `app.enable('foo')`, or `app.option('foo', false)`.
 *
 * ```js
 * {enable: 'foo'}
 * // or
 * {enable: ['foo', 'bar']}
 * ```
 * @name enable
 * @api public
 */

module.exports = function(app, base, options) {
  return function(val, key, config, schema) {
    if (typeof val === 'string') {
      val = [val];
    }
    if (Array.isArray(val)) {
      val.forEach(function(key) {
        app.enable(key);
      });
    }
    return val;
  };
};
