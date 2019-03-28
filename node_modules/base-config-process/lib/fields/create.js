'use strict';

var path = require('path');
var debug = require('debug')('base:cli:process');

/**
 * Create the given template collections.
 *
 * ```json
 * {
 *   "name": "my-project",
 *   "verb": {
 *     "create": {
 *       "pages": {
 *         "cwd": "templates/pages"
 *       },
 *       "posts": {
 *         "cwd": "templates/posts"
 *       }
 *     }
 *   }
 * }
 * ```
 * @name cwd
 * @api public
 */

module.exports = function(app) {
  return function(collections, key, config, next) {
    debug('command > %s: "%j"', key, collections);

    for (var prop in collections) {
      var options = collections[prop];
      if (typeof options.cwd === 'string') {
        options.cwd = path.resolve(options.cwd);
      }
      app.create(prop, options);
    }

    next();
  };
};
