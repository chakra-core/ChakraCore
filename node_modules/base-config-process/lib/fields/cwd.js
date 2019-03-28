'use strict';

var debug = require('../debug');
var path = require('path');

/**
 * Set the current working directory.
 *
 * ```json
 * {
 *   "name": "my-project",
 *   "verb": {
 *     "cwd": "foo/bar"
 *   }
 * }
 * ```
 * @name cwd
 * @api public
 */

module.exports = function(app) {
  var cwds = [app.cwd || process.cwd()];

  return function(val, key, config, next) {
    debug.field(key, val);
    val = path.resolve(val);
    if (cwds[cwds.length - 1] !== val) {
      app.cwd = val;
      cwds.push(val);
    }
    next();
  };
};

