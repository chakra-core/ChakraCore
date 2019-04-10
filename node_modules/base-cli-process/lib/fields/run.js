'use strict';

/**
 * For tasks to run, regardless of other options passed on the command line.
 *
 * ```json
 * $ app --run
 * ```
 * @name run
 * @api public
 */

module.exports = function(app) {
  return function(val, key, config, next) {
    if (val === false) {
      config.tasks = [];
    }
    next();
  };
};
