'use strict';

var debug = require('../debug');

/**
 * Enable or disable Table of Contents rendering, or pass options on the
 * `verb` config object in `package.json`.
 *
 * ```json
 * {
 *   "name": "my-project",
 *   "verb": {
 *     "toc": true
 *   }
 * }
 * ```
 * @name toc
 * @api public
 */

module.exports = function(app) {
  return function(val, key, config, next) {
    debug.field(key, val);
    app.option('toc', val);
    next();
  };
};
