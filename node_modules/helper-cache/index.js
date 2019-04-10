'use strict';

var lazy = require('lazy-cache')(require);
lazy('extend-shallow', 'extend');
lazy('lodash.bind', 'bind');

/**
 * Create an instance of `HelperCache`, optionally passing
 * default `options`.
 *
 * ```js
 * var HelperCache = require('helper-cache');
 * var helpers = new HelperCache();
 * ```
 *
 * @param {Object} `options` Default options to use.
 *   @option {Boolean} [options] `bind` Bind functions to `this`. Defaults to `false`.
 *   @option {Boolean} [options] `thisArg` The context to use.
 * @api public
 */

function HelperCache(opts) {
  if (!(this instanceof HelperCache)) {
    return new HelperCache(opts);
  }

  defineGetter(this, 'options', function () {
    return lazy.extend({bind: false, thisArg: null }, opts);
  });
}

/**
 * Register a helper.
 *
 * ```js
 * helpers.addHelper('lower', function(str) {
 *   return str.toLowerCase();
 * });
 * ```
 *
 * @name .addHelper
 * @param {String} `name` The name of the helper.
 * @param {Function} `fn` Helper function.
 * @return {Object} Return `this` to enable chaining
 * @api public
 */

defineGetter(HelperCache.prototype, 'addHelper', function () {
  return function (name, fn, thisArg) {
    thisArg = thisArg || this.options.thisArg;

    // `addHelpers` handles functions
    if (typeof name === 'function') {
      return this.addHelpers.call(this, arguments);
    }

    if (typeof name === 'object') {
      for (var key in name) {
        this.addHelper(key, name[key], thisArg);
      }
    } else {
      // when `thisArg` and binding is turned on
      if (this.options.bind && typeof thisArg === 'object') {
        if (typeof fn === 'object') {
          var res = {};
          for (var prop in fn) {
            if (fn.hasOwnProperty(prop)) {
              res[prop] = lazy.bind(fn[prop], thisArg);
            }
          }
          this[name] = res;
        } else {
          this[name] = lazy.bind(fn, thisArg);
        }
      } else {
        this[name] = fn;
      }
    }

    // chaining
    return this;
  }.bind(this);
});

/**
 * Register an async helper.
 *
 * ```js
 * helpers.addAsyncHelper('foo', function (str, callback) {
 *   callback(null, str + ' foo');
 * });
 * ```
 *
 * @name .addAsyncHelper
 * @param {String} `key` The name of the helper.
 * @param {Function} `fn` Helper function.
 * @return {Object} Return `this` to enable chaining
 * @api public
 */

defineGetter(HelperCache.prototype, 'addAsyncHelper', function () {
  return function(name, fn, thisArg) {
    // `addAsyncHelpers` handles functions
    if (typeof name === 'function') {
      return this.addAsyncHelpers.call(this, arguments);
    }

    // pass each key/value pair to `addAsyncHelper`
    if (typeof name === 'object') {
      for (var key in name) {
        if (name.hasOwnProperty(key)) {
          this.addAsyncHelper(key, name[key], thisArg);
        }
      }
    } else {
      // when `thisArg` and binding is turned on
      if (this.options.bind && typeof thisArg === 'object') {
        if (typeof fn === 'object') {
          var res = {};
          for (var prop in fn) {
            if (fn.hasOwnProperty(prop)) {
              var val = fn[prop];
              val.async = true;
              res[prop] = lazy.bind(val, thisArg);
            }
          }
          this[name] = res;
        } else {
          fn.async = true;
          this[name] = lazy.bind(fn, thisArg);
        }
      } else {
        fn.async = true;
        this[name] = fn;
      }
    }

    return this;
  }.bind(this);
});

/**
 * Load an object of helpers.
 *
 * ```js
 * helpers.addHelpers({
 *   a: function() {},
 *   b: function() {},
 *   c: function() {},
 * });
 * ```
 *
 * @name .addHelpers
 * @param {String} `key` The name of the helper.
 * @param {Function} `fn` Helper function.
 * @return {Object} Return `this` to enable chaining.
 * @api public
 */

defineGetter(HelperCache.prototype, 'addHelpers', function () {
  return function (helpers, thisArg) {
    thisArg = thisArg || this.options.thisArg;

    // when a function is passed, execute it and use the results
    if (typeof helpers === 'function') {
      return this.addHelpers(helpers(thisArg), thisArg);
    }

    // allow binding each helper if enabled
    for (var key in helpers) {
      if (helpers.hasOwnProperty(key)) {
        this.addHelper(key, helpers[key], thisArg);
      }
    }
    return this;
  }.bind(this);
});

/**
 * Load an object of async helpers.
 *
 * ```js
 * helpers.addAsyncHelpers({
 *   a: function() {},
 *   b: function() {},
 *   c: function() {},
 * });
 * ```
 *
 * @name .addAsyncHelpers
 * @param {String} `key` The name of the helper.
 * @param {Function} `fn` Helper function.
 * @return {Object} Return `this` to enable chaining
 * @api public
 */

defineGetter(HelperCache.prototype, 'addAsyncHelpers', function () {
  return function (helpers, thisArg) {
    // when a function is passed, execute it and use the results
    if (typeof helpers === 'function') {
      thisArg = thisArg || this.options.thisArg;
      return this.addAsyncHelpers(helpers(thisArg), thisArg);
    }

    if (typeof helpers === 'object') {
      for (var key in helpers) {
        if (helpers.hasOwnProperty(key)) {
          this.addAsyncHelper(key, helpers[key], thisArg);
        }
      }
    }
    return this;
  }.bind(this);
});

/**
 * Get a registered helper.
 *
 * ```js
 * helpers.getHelper('foo');
 * ```
 *
 * @name .getHelper
 * @param  {String} `key` The helper to get.
 * @return {Object} The specified helper. If no `key` is passed, the entire cache is returned.
 * @api public
 */

defineGetter(HelperCache.prototype, 'getHelper', function () {
  return function(key) {
    return typeof key === 'string' ? this[key] : this;
  }.bind(this);
});

/**
 * Utility method to define getters.
 *
 * @param  {Object} `obj`
 * @param  {String} `name`
 * @param  {Function} `getter`
 * @return {Getter}
 * @api private
 */

function defineGetter(obj, name, getter) {
  Object.defineProperty(obj, name, {
    configurable: false,
    enumerable: false,
    get: getter,
    set: function() {
      throw new Error(name + ' is a read-only getter.');
    }
  });
}

/**
 * Expose `HelperCache`
 */

module.exports = HelperCache;
