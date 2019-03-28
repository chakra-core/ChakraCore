'use strict';

var debug = require('../debug');
var utils = require('../utils');

/**
 * Merge data onto the `app.cache.data` object. If the [base-data][] plugin
 * is registered, this is the API-equivalent of calling `app.data()`.
 *
 * ```json
 * {
 *   "name": "my-project",
 *   "verb": {
 *     "data": {
 *       "foo": "bar"
 *     }
 *   }
 * }
 * ```
 * @name data
 * @api public
 */

module.exports = function(app) {
  return function(val, key, config, next) {
    debug.field(key, val);

    if (typeof app.data === 'function') {
      app.data(val);

    } else {
      app.cache.data = utils.merge({}, app.cache.data, val);

      for (var prop in val) {
        app.emit('data', prop, val[prop]);
      }
    }
    next();
  };
};
