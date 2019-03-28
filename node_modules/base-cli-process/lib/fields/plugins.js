'use strict';

var utils = require('../utils');

/**
 * Load plugins from a filepath or glob pattern.
 *
 * ```sh
 * $ app --plugins="./foo.js"
 * ```
 * @name plugins
 * @api public
 */

module.exports = function(app) {
  return function(val, key, config, next) {
    if (typeof app.pipeline !== 'function') {
      next(new Error('expected base-pipeline to be registered'));
      return;
    }

    if (utils.isString(val)) {
      val = config[key] = [utils.stripQuotes(val)];
    }

    app.config.process({plugins: val}, next);
  };
};
