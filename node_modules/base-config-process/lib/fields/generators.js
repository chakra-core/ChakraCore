'use strict';

var debug = require('debug')('base:cli:process');

/**
 * _(Requires [templates][], otherwise ignored)_
 *
 * Register generators to use in templates. Value can be a string or
 * array of module names, glob patterns, or file paths, or an object
 * where each key is a filepath, glob or module name, and the value
 * is an options object to pass to resolved generators.
 *
 * _(Modules must be locally installed and listed in `dependencies` or
 * `devDependencies`)_.
 *
 * ```json
 * // module names
 * {
 *   "verb": {
 *     "generators": {
 *       "generator-foo": {},
 *       "generator-bar": {}
 *     }
 *   }
 * }
 *
 * // register a glob of generators
 * {
 *   "verb": {
 *     "generators": ["foo/*.js"]
 *   }
 * }
 * ```
 * @name generators
 * @api public
 */

module.exports = function(app, base, env, options) {
  return function(generators, key, config, next) {
    if (typeof app.generator !== 'function') {
      next(new Error('expected app.generator to be a function'));
      return;
    }

    for (var prop in generators) {
      debug('loading generator "%s"', prop);
      this.register(prop, generators[prop].fn);
    }

    next();
  };
};
