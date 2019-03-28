'use strict';

var debug = require('../debug');

/**
 * Define plugins to load. Value can be a string or array of module names.
 *
 * _(Modules must be locally installed and listed in `dependencies` or
 * `devDependencies`)_.
 *
 * ```json
 * {"verb":  {"use": ["base-option", "base-data"]}}
 * ```
 * @name use
 * @api public
 */

module.exports = function(app) {
  return function(val, key, config, next) {
    debug.field(key, val);

    for (var prop in val) {
      app.use(val[prop].fn);
    }
    next();
  };
};
