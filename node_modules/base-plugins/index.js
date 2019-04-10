/*!
 * base-plugins <https://github.com/jonschlinkert/base-plugins>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isRegistered = require('is-registered');
var define = require('define-property');
var isObject = require('isobject');

module.exports = function plugin() {
  return function(app) {
    if (isRegistered(app, 'base-plugins')) return;
    if (!app.fns) {
      define(app, 'fns', []);
    }

    /**
     * Define a plugin function to be called immediately upon init.
     * The only parameter exposed to the plugin is the application
     * instance.
     *
     * Also, if a plugin returns a function, the function will be pushed
     * onto the `fns` array, allowing the plugin to be called at a
     * later point, elsewhere in the application.
     *
     * ```js
     * // define a plugin
     * function foo(app) {
     *   // do stuff
     * }
     *
     * // register plugins
     * var app = new Base()
     *   .use(foo)
     *   .use(bar)
     *   .use(baz)
     * ```
     * @name .use
     * @param {Function} `fn` plugin function to call
     * @return {Object} Returns the item instance for chaining.
     * @api public
     */

    define(app, 'use', use);

    /**
     * Run all plugins
     *
     * ```js
     * var config = {};
     * app.run(config);
     * ```
     * @name .run
     * @param {Object} `value` Object to be modified by plugins.
     * @return {Object} Returns the item instance for chaining.
     * @api public
     */

    define(app, 'run', function(val) {
      if (!isObject(val)) return;
      decorate(val);

      var len = this.fns.length;
      var idx = -1;
      while (++idx < len) {
        val.use(this.fns[idx]);
      }
      return this;
    });
  };

  /**
   * Prime the `.use` method and `fns`
   * array on `val`
   */

  function decorate(val) {
    if (!val.use) {
      define(val, 'fns', val.fns || []);
      define(val, 'use', use);
      val.use(plugin());
    }
  }

  /**
   * Call plugin `fn`. If a function is returned push it into the
   * `fns` array to be called by the `run` method.
   */

  function use(fn, options) {
    var val = fn.call(this, this, this.base || {}, this.env || {}, options || {});
    if (typeof val === 'function') {
      this.fns.push(val);
    }
    if (this.emit) {
      this.emit('use', val, this);
    }
    return this;
  }
};
