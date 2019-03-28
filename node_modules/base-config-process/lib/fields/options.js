'use strict';

var debug = require('../debug');

/**
 * Merge options onto the `app.options` object. If the [base-option][] plugin
 * is registered, this is the API-equivalent of calling `app.option()`.
 *
 * ```json
 * {"verb": {"options": {"foo": "bar"}}}
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
