/*!
 * use <https://github.com/jonschlinkert/use>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var define = require('define-property');
var isObject = require('isobject');

module.exports = function base(app) {
  if (!app.fns) {
    define(app, 'fns', []);
  }

  /**
   * Define a plugin function to be passed to use. The only
   * parameter exposed to the plugin is `app`, the object or function.
   * passed to `use(app)`. `app` is also exposed as `this` in plugins.
   *
   * Additionally, **if a plugin returns a function, the function will
   * be pushed onto the `fns` array**, allowing the plugin to be
   * called at a later point by the `run` method.
   *
   * ```js
   * var use = require('use');
   *
   * // define a plugin
   * function foo(app) {
   *   // do stuff
   * }
   *
   * var app = function(){};
   * use(app);
   *
   * // register plugins
   * app.use(foo);
   * app.use(bar);
   * app.use(baz);
   * ```
   * @name .use
   * @param {Function} `fn` plugin function to call
   * @api public
   */

  define(app, 'use', use);

  /**
   * Run all plugins on `fns`. Any plugin that returns a function
   * when called by `use` is pushed onto the `fns` array.
   *
   * ```js
   * var config = {};
   * app.run(config);
   * ```
   * @name .run
   * @param {Object} `value` Object to be modified by plugins.
   * @return {Object} Returns the object passed to `run`
   * @api public
   */

  define(app, 'run', function (val) {
    decorate(val);
    var len = this.fns.length, i = -1;
    while (++i < len) val.use(this.fns[i]);
    return val;
  });

  /**
   * Call plugin `fn`. If a function is returned push it into the
   * `fns` array to be called by the `run` method.
   */

  function use(fn) {
    var plugin = fn.call(this, this);
    if (typeof plugin === 'function') {
      this.fns.push(plugin);
    }
    return this;
  }

  /**
   * Ensure the `.use` method exists on `val`
   */

  function decorate(val) {
    if (isObject(val) && (!val.use || !val.run)) {
      base(val);
    }
  }

  return app;
};
