'use strict';

/**
 * Enable a configuration setting. Also pass `-c` to save the
 * value to the `verb.config` object in package.json.
 *
 * ```sh
 * $ app --enable toc
 * ```
 * @name enable
 * @api public
 */

module.exports = function(app) {
  return function(prop, key, config, next) {

    if (prop === 'toc') {
      app.base.enable('toc.render');
      if (config.c === true) {
        app.pkg.set('verb.toc', true);
        app.pkg.save();
      }
    } else {
      app.base.enable(prop);
      if (config.c === true) {
        app.pkg.set(prop, true);
        app.pkg.save();
      }
    }
    next();
  };
};
