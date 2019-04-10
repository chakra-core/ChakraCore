'use strict';

var debug = require('debug')('base:cli:process');

/**
 * Load pipeline plugins. Requires the [base-pipeline][] plugin to be
 * registered.
 *
 * _(Modules must be locally installed and listed in `dependencies` or
 * `devDependencies`)_.
 *
 * ```json
 * {"verb": {"plugins": {"eslint": {"name": "gulp-eslint"}}}}
 * ```
 * @name plugins
 * @api public
 */

module.exports = function(app) {
  return function(plugins, key, config, next) {
    if (typeof app.plugin === 'undefined') {
      next();
      return;
    }

    for (var prop in plugins) {
      debug('loading plugin "%s"', prop);
      app.plugin(prop, plugins[prop].fn);
    }
    next();
  };
};
