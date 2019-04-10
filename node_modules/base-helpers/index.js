/*!
 * base-helpers (https://github.com/node-base/base-helpers)
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var debug = require('debug')('base:helpers');
var utils = require('./utils');

module.exports = function() {
  return function(app) {
    if (!utils.isValid(app)) return;

    if (typeof app._ === 'undefined') {
      utils.define(app, '_', {});
    }

    if (typeof app._.helpers === 'undefined') {
      app._.helpers = {async: {}, sync: {}};
    }

    /**
     * Create loader objects
     */

    var async = utils.loader(app._.helpers.async, {async: true});
    var sync = utils.loader(app._.helpers.sync);

    /**
     * Register a template helper.
     *
     * ```js
     * app.helper('upper', function(str) {
     *   return str.toUpperCase();
     * });
     * ```
     * @name .helper
     * @param {String} `name` Helper name
     * @param {Function} `fn` Helper function.
     * @api public
     */

    app.define('helper', function(name) {
      debug('registering sync helper "%s"', name);
      sync.apply(sync, arguments);
      return this;
    });

    /**
     * Register multiple template helpers.
     *
     * ```js
     * app.helpers({
     *   foo: function() {},
     *   bar: function() {},
     *   baz: function() {}
     * });
     * ```
     * @name .helpers
     * @param {Object|Array} `helpers` Object, array of objects, or glob patterns.
     * @api public
     */

    app.define('helpers', function(name, helpers) {
      if (typeof name === 'string' && utils.isHelperGroup(helpers)) {
        return this.helperGroup.apply(this, arguments);
      }
      sync.apply(sync, arguments);
      return this;
    });

    /**
     * Register an async helper.
     *
     * ```js
     * app.asyncHelper('upper', function(str, next) {
     *   next(null, str.toUpperCase());
     * });
     * ```
     * @name .asyncHelper
     * @param {String} `name` Helper name.
     * @param {Function} `fn` Helper function
     * @api public
     */

    app.define('asyncHelper', function(name) {
      debug('registering async helper "%s"', name);
      async.apply(async, arguments);
      return this;
    });

    /**
     * Register multiple async template helpers.
     *
     * ```js
     * app.asyncHelpers({
     *   foo: function() {},
     *   bar: function() {},
     *   baz: function() {}
     * });
     * ```
     * @name .asyncHelpers
     * @param {Object|Array} `helpers` Object, array of objects, or glob patterns.
     * @api public
     */

    app.define('asyncHelpers', function(name, helpers) {
      if (typeof name === 'string' && utils.isHelperGroup(helpers)) {
        return this.helperGroup.apply(this, arguments);
      }
      async.apply(async, arguments);
      return this;
    });

    /**
     * Get a previously registered helper.
     *
     * ```js
     * var fn = app.getHelper('foo');
     * ```
     * @name .getHelper
     * @param {String} `name` Helper name
     * @returns {Function} Returns the registered helper function.
     * @api public
     */

    app.define('getHelper', function(name) {
      debug('getting sync helper "%s"', name);
      return this.get(['_.helpers.sync', name]);
    });

    /**
     * Get a previously registered async helper.
     *
     * ```js
     * var fn = app.getAsyncHelper('foo');
     * ```
     * @name .getAsyncHelper
     * @param {String} `name` Helper name
     * @returns {Function} Returns the registered helper function.
     * @api public
     */

    app.define('getAsyncHelper', function(name) {
      debug('getting async helper "%s"', name);
      return this.get(['_.helpers.async', name]);
    });

    /**
     * Return true if sync helper `name` is registered.
     *
     * ```js
     * if (app.hasHelper('foo')) {
     *   // do stuff
     * }
     * ```
     * @name .hasHelper
     * @param {String} `name` sync helper name
     * @returns {Boolean} Returns true if the sync helper is registered
     * @api public
     */

    app.define('hasHelper', function(name) {
      return typeof this.getHelper(name) === 'function';
    });

    /**
     * Return true if async helper `name` is registered.
     *
     * ```js
     * if (app.hasAsyncHelper('foo')) {
     *   // do stuff
     * }
     * ```
     * @name .hasAsyncHelper
     * @param {String} `name` Async helper name
     * @returns {Boolean} Returns true if the async helper is registered
     * @api public
     */

    app.define('hasAsyncHelper', function(name) {
      return typeof this.getAsyncHelper(name) === 'function';
    });

    /**
     * Register a namespaced helper group.
     *
     * ```js
     * // markdown-utils
     * app.helperGroup('mdu', {
     *   foo: function() {},
     *   bar: function() {},
     * });
     *
     * // Usage:
     * // <%%= mdu.foo() %>
     * // <%%= mdu.bar() %>
     * ```
     * @name .helperGroup
     * @param {Object|Array} `helpers` Object, array of objects, or glob patterns.
     * @api public
     */

    app.define('helperGroup', function(name, helpers, isAsync) {
      debug('registering helper group "%s"', name);
      var type = isAsync ? 'async' : 'sync';

      var group = this._.helpers[type][name] || (this._.helpers[type][name] = {});
      if (typeof helpers === 'function' && utils.isHelperGroup(helpers)) {
        Object.defineProperty(helpers, 'isGroup', {
          enumerable: false,
          configurable: false,
          value: true
        });

        if (isAsync === true) {
          decorateHelpers(helpers, helpers, isAsync);
        }

        decorateHelpers(helpers, group, isAsync, true);
        this._.helpers[type][name] = helpers;
        return this;
      }

      helpers = utils.arrayify(helpers);
      var loader = utils.loader(group, {async: isAsync});
      loader.call(loader, helpers);
      return this;
    });
  };
};

function decorateHelpers(oldHelpers, newHelpers, isAsync, override) {
  for (let key in newHelpers) {
    if (newHelpers.hasOwnProperty(key)) {
      // "newHelpers" is the helpers being passed by the user
      if (override === true && oldHelpers[key]) continue;
      let fn = newHelpers[key];
      if (typeof fn === 'function') {
        fn.async = isAsync;
      }
      oldHelpers[key] = fn;
    }
  }
}

