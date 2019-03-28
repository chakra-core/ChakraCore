'use strict';

var path = require('path');
var debug = require('../debug');
var utils = require('../utils');

/**
 * Set the `--cwd` to use in the current project.
 *
 * ```sh
 * $ app --cwd=foo
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
      var dir = utils.magenta('~/' + utils.homeRelative(val));
      utils.timestamp('changing cwd to ' + dir);
      cwds.push(val);
    }
    next();
  };
};

