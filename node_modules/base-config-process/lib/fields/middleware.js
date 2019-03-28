'use strict';

var debug = require('debug')('base:cli:process:middleware');
var utils = require('../utils');

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
  return function(obj, name, config, next) {
    for (var key in obj) {
      if (typeof app[key] !== 'function') {
        continue;
      }

      if (obj.hasOwnProperty(key)) {
        var arr = obj[key];
        var len = arr.length;
        var idx = -1;
        while (++idx < len) {
          var val = arr[idx];
          debug('adding middleware <%s> to stage <%s>', val.name, key);
          var re = utils.toRegex(val.regex || val.match || /./);
          app[key](re, val.fn(val.options));
        }
      }
    }
    next();
  };
};
