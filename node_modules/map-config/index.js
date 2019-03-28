/*!
 * map-config <https://github.com/doowb/map-config>
 *
 * Copyright (c) 2015, Brian Woodward.
 * Licensed under the MIT License.
 */

'use strict';

var unique = require('array-unique');
var async = require('async');

/**
 * Create a new instance of MapConfig with a specified map and application.
 *
 * ```js
 * var mapper = new MapConfig(app, map);
 * ```
 * @param {Object} `app` Object containing the methods that will be called based on the map specification.
 * @param {Object} `map` Optional object specifying how to map a configuration to an application.
 * @api public
 */

function MapConfig(app, config) {
  if (!(this instanceof MapConfig)) {
    return new MapConfig(app, config);
  }

  define(this, 'isMapConfig', true);

  this.app = app || {};
  this.keys = [];
  this.aliases = {};
  this.config = {};
  this.async = false;

  if (config && typeof config === 'object') {
    for (var key in config) {
      this.map(key, config[key]);
    }
  }
}

/**
 * Map properties to methods and/or functions.
 *
 * ```js
 * mapper
 *   .map('baz')
 *   .map('bang', function(val, key, config) {
 *   });
 * ```
 *
 * @param  {String} `key` property key to map.
 * @param  {Function|Object} `val` Optional function to call when a config has the given key.
 *                                 Functions will be passed `(val, key, config)` when called.
 *                                 Functions may also take a callback to indicate async usage.
 *                                 May also pass another instance of MapConfig to be processed.
 * @return {Object} `this` to enable chaining
 * @api public
 */

MapConfig.prototype.map = function(key, val) {
  if (isMapConfig(val)) {
    return this.subConfig.apply(this, arguments);
  }

  if (typeof val === 'string') {
    return this.alias.apply(this, arguments);
  }

  if (typeof val === 'function') {
    if (hasCallback(val)) {
      this.async = true;
      val.async = true;
    }
  } else {
    val = this.app[key];
  }

  this.config[key] = val;
  this.addKey(key);
  return this;
};

/**
 * Private method for mapping another map-config instance to `key`.
 *
 * ```js
 * var one = new MapConfig();
 * one.map('foo', function(val) {
 *   // do stuff
 * });
 *
 * var two = new MapConfig();
 * two.subConfig('bar', one);
 * ```
 * @param {String} `prop` The property name to map to `sub`
 * @param {Object} `sub` Instance of map-config
 * @return {Object} Returns the instance for chaining.
 */

MapConfig.prototype.subConfig = function(prop, sub) {
  this.map(prop, function(val, key, config, next) {
    sub.process(val, next);
  });
  return this.addKey(prop, sub.keys);
};

/**
 * Create an `alias` for property `key`.
 *
 * ```js
 * mapper.alias('foo', 'bar');
 * ```
 * @param  {String} `alias` Alias to use for `key`.
 * @param  {String} `key` Actual property or method on `app`.
 * @return {Object} Returns the instance for chaining.
 * @api public
 */

MapConfig.prototype.alias = function(alias, key) {
  if (this.config.hasOwnProperty(key)) {
    this.config[alias] = this.config[key];
  } else {
    this.aliases[alias] = key;
    this.addKey(alias);
  }
  return this;
};

/**
 * Process a configuration object with the already configured `map` and `app`.
 *
 * ```js
 * mapper.process(config);
 * ```
 * @param  {Object} `config` Configuration object to map to application methods.
 * @param  {Function} `cb` Optional callback function that will be called when finished or if an error occurs during processing.
 * @api public
 */

MapConfig.prototype.process = function(args, cb) {
  if (typeof args === 'function') {
    cb = args;
    args = null;
  }

  for (var key in this.aliases) {
    var alias = this.aliases[key];
    this.map(key, this.config[alias] || this.app[alias]);
  }

  if (typeof cb !== 'function') {
    if (this.async === true) {
      throw new Error('expected a callback function');
    }
    cb = function(err) {
      if (err) throw err;
    };
  }

  args = args || {};
  async.eachOfSeries(args, function(val, key, next) {
    var fn = this.config[key];
    if (typeof fn !== 'function') {
      return next();
    }

    if (fn.async === true) {
      return fn.call(this.app, val, key, args, next);
    }

    try {
      fn.call(this.app, val, key, args);
      return next();
    } catch (err) {
      err.message = 'map-config#process ' + err.message;
      return next(err);
    }
  }.bind(this), cb.bind(this.app));
};

/**
 * Add a key to the `.keys` array. May also be used to add
 * an array of namespaced keys to the `.keys` array. Useful
 * for mapping "sub-configs" to a key in a parent config.
 *
 * ```js
 * mapper.addKey('foo');
 * console.log(mapper.keys);
 * //=> ['foo']
 *
 * var one = new MapConfig();
 * var two = new MapConfig();
 * two.map('foo');
 * two.map('bar');
 * two.map('baz');
 *
 * // map config `two` to config `one`
 * one.map('two', function(val, key, config, next) {
 *   two.process(val, next);
 * });
 *
 * // map keys from config `two` to config `one`
 * one.addKey('two', two.keys);
 * console.log(one.keys);
 * //=> ['two.foo', 'two.bar', 'two.baz']
 * ```
 *
 * @param {String} `key` key to push onto `.keys`
 * @param {Array} `arr` Array of sub keys to push onto `.keys`
 * @return {Object} `this` for chaining
 * @api public
 */

MapConfig.prototype.addKey = function(key, arr) {
  var keys = this.keys.slice();
  if (Array.isArray(arr)) {
    keys = keys.concat(joinKeys(key, arr));
    var i = keys.indexOf(key);
    if (i !== -1) {
      keys.splice(i, 1);
    }
  } else {
    keys.push(key);
  }

  this.keys = unique(keys);
  return this;
};

/**
 * Check if given value looks like an instance of `map-config`
 *
 * @param  {*} `val` Value to check
 * @return {Boolean} `true` if instance of `map-config`
 */

function isMapConfig(val) {
  return val
    && typeof val === 'object'
    && val.isMapConfig === true;
}

/**
 * Return true if the given `fn` has a callback.
 *
 * @param {String} str
 * @return {Boolean}
 */

function hasCallback(fn) {
  var re = /^function[ \t]*([\w\W]+?, *(cb|callback|next))/;
  return re.test(fn.toString());
}

/**
 * Join "child" `keys` to a `parent` namespace.
 *
 * @param {String} parent
 * @param {Array} keys
 * @return {Array}
 */

function joinKeys(parent, keys) {
  var len = keys.length;
  var idx = -1;
  var arr = [];

  while (++idx < len) {
    arr.push(parent + '.' + keys[idx]);
  }
  return arr;
}

/**
 * Define a property on an object.
 *
 * @param  {Object} `obj` Object to define property on.
 * @param  {String} `prop` Property to define.
 * @param  {*} `val` Value of property to define.
 */

function define(obj, prop, val) {
  Object.defineProperty(obj, prop, {
    enumerable: false,
    configurable: true,
    value: val
  });
}

/**
 * Exposes MapConfig
 */

module.exports = MapConfig;
