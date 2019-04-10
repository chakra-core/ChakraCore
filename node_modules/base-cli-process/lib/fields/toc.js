'use strict';

var debug = require('../debug');

/**
 * Enable or disable Table of Contents rendering
 *
 * ```sh
 * # enable
 * $ app --toc
 * # or
 * $ app --toc=true
 * # disable
 * $ app --toc=false
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
