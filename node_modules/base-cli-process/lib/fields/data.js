'use strict';

var debug = require('../debug');
var utils = require('../utils');

/**
 * Define data to be used for rendering templates.
 *
 * ```sh
 * $ app --data=foo:bar
 * # {foo: 'bar'}
 *
 * $ app --data=foo.bar:baz
 * # {foo: {bar: 'baz'}}
 *
 * $ app --data=foo:bar,baz
 * # {foo: ['bar', 'baz']}}
 * ```
 * @name data
 * @api public
 */

module.exports = function(app, base) {
  return function(val, key, config, next) {
    debug.field(key, val);

    if (utils.show(val)) {
      console.log(utils.formatValue(app.cache.data));
      next();
      return;
    }

    if (typeof app.data === 'function') {
      app.data(val);

    } else {
      for (var prop in val) {
        app.emit('data', prop, val[prop]);
      }
      app.cache.data = utils.merge({}, app.cache.data, val);
      val = app.cache.data;
    }

    config[key] = val;
    next();
  };
};
