'use strict';

/**
 * Conditionally runs a callback based upon the
 * current environment.
 *
 * @example
 * const env = require('broccoli-stew').env;
 * const uglify = require('broccoli-uglify-js');
 * const dist = 'lib';
 *
 * env('production', function() {
 *   dist = uglify(dist);
 * });
 *
 * env('!development', function() {
 *   dist = es3Recast(dist);
 * });
 *
 * env('development', 'test', function() {
 *   dist = createTest(dist);
 * });
 *
 * module.exports = dist;
 *
 * @param {String} envName... The names of environments that are used for gating the callback
 * @param {Function} cb The callback that is run when the environment matches
 */
module.exports = function env(/* envName..., cb */) {
  let length = arguments.length;
  let envs = Array.prototype.slice.call(arguments).slice(0, length - 1);
  let cb = arguments[length - 1];

  let currentEnv = process.env.BROCCOLI_ENV ||
                   process.env.EMBER_ENV    ||
                   'development';

  let match = envs.map(name => {
    if (name.charAt(0) === '!') {
      return name.substring(1) !== currentEnv;
    } else {
      return name == currentEnv;
    }
  }).filter(Boolean).length > 0;

  if (match) {
    return cb();
  }
};
