/*!
 * option-cache <https://github.com/jonschlinkert/option-cache>
 *
 * Copyright (c) 2014-2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var Emitter = require('component-emitter');
var utils = require('./utils');

/**
 * Create a new instance of `Options`.
 *
 * ```js
 * var app = new Options();
 * ```
 *
 * @param {Object} `options` Initialize with default options.
 * @api public
 */

function Options(options) {
  if (!(this instanceof Options)) {
    return new Options(options);
  }
  this.defaults = this.defaults || {};
  this.options = this.options || {};
  if (options) {
    this.option(options);
  }
}

/**
 * `Options` prototype methods.
 */

Options.prototype = Emitter({
  constructor: Options,

  /**
   * Set or get an option.
   *
   * ```js
   * app.option('a', true);
   * app.option('a');
   * //=> true
   * ```
   * @name .option
   * @param {String} `key` The option name.
   * @param {*} `value` The value to set.
   * @return {*} Returns a `value` when only `key` is defined.
   * @api public
   */

  option: function(key, value) {
    if (typeof key === 'undefined') return;
    if (Array.isArray(key)) {
      if (arguments.length > 1) {
        key = utils.toPath(key);

      } else if (typeof key[0] === 'string') {
        key = utils.toPath(arguments);
      }
    }

    var type = utils.typeOf(key);
    if (type === 'string') {
      if (arguments.length === 1) {
        return this.either(key, utils.get(this.defaults, key));
      }
      utils.set(this.options, key, value);
      this.emit('option', key, value);
      return this;
    }

    if (type !== 'object' && type !== 'array') {
      var msg = 'expected option to be a string, object or array';
      throw new TypeError(msg);
    }
    return this.mergeOptions.apply(this, arguments);
  },

  /**
   * Set or get a default value. Defaults are cached on the `.defaults`
   * object.
   *
   * ```js
   * app.default('admin', false);
   * app.default('admin');
   * //=> false
   *
   * app.option('admin');
   * //=> false
   *
   * app.option('admin', true);
   * app.option('admin');
   * //=> true
   * ```
   * @name .option
   * @param {String} `key` The option name.
   * @param {*} `value` The value to set.
   * @return {*} Returns a `value` when only `key` is defined.
   * @api public
   */

  default: function(prop, val) {
    switch (utils.typeOf(prop)) {
      case 'object':
        this.visit('default', prop);
        return this;
      case 'string':
        if (typeof val !== 'undefined') {
          utils.set(this.defaults, prop, val);
          return this;
        }
        return utils.get(this.defaults, prop);
      default: {
        throw new TypeError('expected a string or object');
      }
    }
  },

  /**
   * Returns the value of `key` or `value`, Or, if `type` is passed
   * and the value of `key` is not the same javascript native type
   * as `type`, then `value` is returned.
   *
   * ```js
   * app.option('admin', true);
   * console.log(app.either('admin', false));
   * //=> true
   *
   * console.log(app.either('collaborator', false));
   * //=> false
   * ```
   * @param {String} `key`
   * @param {any} `value`
   * @param {String} `type` Javascript native type (optional)
   * @return {Object}
   * @api public
   */

  either: function(key, value, type) {
    var val = utils.get(this.options, key);
    if (typeof val === 'undefined' || (type && utils.typeOf(val) !== type)) {
      return value;
    }
    return val;
  },

  /**
   * Set option `key` with the given `value`, but only if `key` is not
   * already defined or the currently defined value isn't the same type
   * as the javascript native `type` (optionally) passed as the third
   * argument.
   *
   * ```js
   * app.option('a', 'b');
   *
   * app.fillin('a', 'z');
   * app.fillin('x', 'y');
   *
   * app.option('a');
   * //=> 'b'
   * app.option('x');
   * //=> 'y'
   * ```
   * @param {String} `key`
   * @param {any} `value`
   * @param {String} `type` Javascript native type (optional)
   * @return {Object}
   * @api public
   */

  fillin: function(prop, value, type) {
    if (utils.typeOf(prop) === 'object') {
      var obj = prop;

      var keys = Object.keys(obj);
      for (var i = 0; i < keys.length; i++) {
        var key = keys[i];
        this.fillin(key, obj[key], type);
      }
    } else {
      var val = this.option(prop);
      if (typeof val === 'undefined' || (typeof type === 'string' && utils.typeOf(val) !== type)) {
        this.option(prop, value);
      }
    }
    return this;
  },

  /**
   * Merge an object, list of objects, or array of objects,
   * onto the `app.options`.
   *
   * ```js
   * app.mergeOptions({a: 'b'}, {c: 'd'});
   * app.option('a');
   * //=> 'b'
   * app.option('c');
   * //=> 'd'
   * ```
   * @param {Object} `options`
   * @return {Object}
   * @api public
   */

  mergeOptions: function(options) {
    var args = [].slice.call(arguments);
    if (Array.isArray(options)) {
      args = utils.flatten(args);
    }

    utils.mergeArray(args, function(val, key) {
      this.emit('option', key, val);
      utils.set(this.options, key, val);
    }, this);
    return this;
  },

  /**
   * Return true if `options.hasOwnProperty(key)`
   *
   * ```js
   * app.hasOption('a');
   * //=> false
   * app.option('a', 'b');
   * app.hasOption('a');
   * //=> true
   * ```
   * @name .hasOption
   * @param {String} `prop`
   * @return {Boolean} True if `prop` exists.
   * @api public
   */

  hasOption: function(key) {
    var prop = utils.toPath(arguments);
    return prop.indexOf('.') === -1
      ? this.options.hasOwnProperty(prop)
      : utils.has(this.options, prop);
  },

  /**
   * Enable `key`.
   *
   * ```js
   * app.enable('a');
   * ```
   * @name .enable
   * @param {String} `key`
   * @return {Object} `Options`to enable chaining
   * @api public
   */

  enable: function(key) {
    this.option(key, true);
    return this;
  },

  /**
   * Disable `key`.
   *
   * ```js
   * app.disable('a');
   * ```
   * @name .disable
   * @param {String} `key` The option to disable.
   * @return {Object} `Options`to enable chaining
   * @api public
   */

  disable: function(key) {
    this.option(key, false);
    return this;
  },

  /**
   * Check if `prop` is enabled (truthy).
   *
   * ```js
   * app.enabled('a');
   * //=> false
   *
   * app.enable('a');
   * app.enabled('a');
   * //=> true
   * ```
   * @name .enabled
   * @param {String} `prop`
   * @return {Boolean}
   * @api public
   */

  enabled: function(key) {
    var prop = utils.toPath(arguments);
    return Boolean(this.option(prop));
  },

  /**
   * Check if `prop` is disabled (falsey).
   *
   * ```js
   * app.disabled('a');
   * //=> true
   *
   * app.enable('a');
   * app.disabled('a');
   * //=> false
   * ```
   * @name .disabled
   * @param {String} `prop`
   * @return {Boolean} Returns true if `prop` is disabled.
   * @api public
   */

  disabled: function(key) {
    var prop = utils.toPath(arguments);
    return !Boolean(this.option(prop));
  },

  /**
   * Returns true if the value of `prop` is strictly `true`.
   *
   * ```js
   * app.option('a', 'b');
   * app.isTrue('a');
   * //=> false
   *
   * app.option('c', true);
   * app.isTrue('c');
   * //=> true
   *
   * app.option({a: {b: {c: true}}});
   * app.isTrue('a.b.c');
   * //=> true
   * ```
   * @name .isTrue
   * @param {String} `prop`
   * @return {Boolean} Uses strict equality for comparison.
   * @api public
   */

  isTrue: function(key) {
    var prop = utils.toPath(arguments);
    return this.option(prop) === true;
  },

  /**
   * Returns true if the value of `key` is strictly `false`.
   *
   * ```js
   * app.option('a', null);
   * app.isFalse('a');
   * //=> false
   *
   * app.option('c', false);
   * app.isFalse('c');
   * //=> true
   *
   * app.option({a: {b: {c: false}}});
   * app.isFalse('a.b.c');
   * //=> true
   * ```
   * @name .isFalse
   * @param {String} `prop`
   * @return {Boolean} Uses strict equality for comparison.
   * @api public
   */

  isFalse: function(key) {
    var prop = utils.toPath(arguments);
    return this.option(prop) === false;
  },

  /**
   * Return true if the value of key is either `true`
   * or `false`.
   *
   * ```js
   * app.option('a', 'b');
   * app.isBoolean('a');
   * //=> false
   *
   * app.option('c', true);
   * app.isBoolean('c');
   * //=> true
   * ```
   * @name .isBoolean
   * @param {String} `key`
   * @return {Boolean} True if `true` or `false`.
   * @api public
   */

  isBoolean: function(key) {
    var prop = utils.toPath(arguments);
    return typeof this.option(prop) === 'boolean';
  },

  /**
   * Visit `method` over each object in the given collection.
   *
   * @param  {String} `method`
   * @param  {Array|Object} `value`
   */

  visit: function(method, collection) {
    utils.visit(this, method, collection);
    return this;
  }
});

/**
 * Expose `Options`
 */

module.exports = Options;
