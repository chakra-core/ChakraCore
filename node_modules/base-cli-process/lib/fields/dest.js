'use strict';

var path = require('path');
var debug = require('../debug');
var utils = require('../utils');

/**
 * Set a `dest` path on `app.options`.
 *
 * ```sh
 * $ app --dest=foo
 * ```
 * @name dest
 * @api public
 */

module.exports = function(app) {
  var dests = app.options.dest ? [app.options.dest] : [app.cwd];

  return function(val, key, config, next) {
    debug.field(key, val);
    app.options.dest = path.resolve(val);

    if (dests[dests.length - 1] !== app.options.dest) {
      var dir = utils.magenta('~/' + utils.homeRelative(app.options.dest));
      utils.timestamp('using dest path ' + dir);
      dests.push(app.options.dest);
    }
    next();
  };
};

