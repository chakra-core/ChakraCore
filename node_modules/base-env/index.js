/*!
 * base-env <https://github.com/base/base-env>
 *
 * Copyright (c) 2015-present, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var debug = require('debug')('base:base-env');
var utils = require('./lib/utils');
var Env = require('./lib/env');

module.exports = function(config) {
  config = config || {};

  return function envPlugin(app) {
    if (!utils.isValid(app)) return;
    app.use(utils.namespace());

    /**
     * Create an `env` object with the given `name`, function, filepath or app instance,
     * and options. See the [Env]() API docs below.
     *
     * ```js
     * var base = require('base');
     * var env = require('base-env');
     * var app = new Base();
     * app.use(env());
     *
     * var env = app.createEnv('foo', function() {});
     * ```
     *
     * @name createEnv
     * @param {String} `name`
     * @param {Object|Function|String} `val`
     * @param {Object} `options`
     * @return {Object}
     * @api public
     */

    app.define('createEnv', function(name, val, options) {
      debug('createEnv: "%s"', name);
      if (!utils.isString(name)) {
        throw new TypeError('expected name to be a string');
      }

      if (name === 'default') {
        return new Env(name, val, this, options);
      }

      if (!utils.isValidArg(val)) {
        val = {};
      }

      if (utils.isAppArg(options)) {
        return this.createEnv(name, options, val);
      }

      if (!utils.isAppArg(val)) {
        return this.createEnv(name, name, utils.extend(val, options));
      }

      if (utils.isValidInstance(options)) {
        return new Env(name, val, options, {});
      } else {
        return new Env(name, val, this, options);
      }
    });

    return envPlugin;
  };
};
