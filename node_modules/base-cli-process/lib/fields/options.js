'use strict';

var debug = require('../debug');

/**
 * Set options. This is the API-equivalent of calling `app.option('foo', 'bar')`.
 *
 * ```sh
 * $ app --options=foo:bar
 * ```
 * @name options
 * @api public
 */

module.exports = function(app) {
  return function(val, key, config, next) {
    debug.field(key, val);
    app.option(val);
    next();
  };
};
