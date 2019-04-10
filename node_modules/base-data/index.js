/*!
 * base-data <https://github.com/node-base/base-data>
 *
 * Copyright (c) 2015-2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var util = require('util');
var cache = require('cache-base');
var Cache = cache.namespace('cache');
var utils = require('./utils');

module.exports = function(prop, config) {
  if (utils.isObject(prop)) {
    config = prop;
    prop = 'cache.data';
  }
  if (typeof prop === 'undefined') {
    prop = 'cache.data';
  }

  return function baseData() {
    if (!utils.isValid(this, prop)) {
      return;
    }

    if (!this.dataLoaders) {
      this.define('dataLoaders', []);
    }

    var cache = this.get(prop);
    if (typeof cache === 'undefined') {
      this.set(prop, {});
      cache = this.get(prop);
    }

    /**
     * Intantiate `Data` using `this[prop]` as the data cache
     */

    var data = new Data(cache);
    cache = data.cache;

    /**
     * Register a data loader for loading data onto `app.cache.data`.
     *
     * ```js
     * var yaml = require('js-yaml');
     *
     * app.dataLoader('yml', function(str, fp) {
     *   return yaml.safeLoad(str);
     * });
     *
     * app.data('foo.yml');
     * //=> loads and parses `foo.yml` as yaml
     * ```
     *
     * @name .dataLoader
     * @param  {String} `ext` The file extension for to match to the loader
     * @param  {Function} `fn` The loader function.
     * @api public
     */

    this.define('dataLoader', function(name, fn) {
      this.dataLoaders.push({name: name, fn: fn});
      return this;
    });

    /**
     * Load data onto `app.cache.data`
     *
     * ```js
     * console.log(app.cache.data);
     * //=> {};
     *
     * app.data('a', 'b');
     * app.data({c: 'd'});
     * console.log(app.cache.data);
     * //=> {a: 'b', c: 'd'}
     *
     * // set an array
     * app.data('e', ['f']);
     *
     * // overwrite the array
     * app.data('e', ['g']);
     *
     * // update the array
     * app.data('e', ['h'], true);
     * console.log(app.cache.data.e);
     * //=> ['g', 'h']
     * ```
     * @name .data
     * @param {String|Object} `key` Key of the value to set, or object to extend.
     * @param {any} `val`
     * @return {Object} Returns the instance of `Template` for chaining
     * @api public
     */

    this.define('data', function(key, value, union) {
      if (typeof key === 'undefined') {
        return this;
      }

      var args = [].slice.call(arguments);
      var last = utils.last(args);
      var self = this;

      this.emit('data', args);
      if (utils.isObject(key)) {
        data.merge.apply(data, arguments);
        return this;
      }

      if (utils.isGlob(key, value)) {
        var opts = utils.merge({}, config, this.options);

        // if the last argument is options, merge in app.options
        if (utils.isObject(last)) {
          opts = utils.merge({}, opts, args.pop());
        }

        // add options to args
        args.push(opts);

        var files = utils.resolve.sync.apply(null, args);
        var len = files.length, i = -1;
        while (++i < len) {
          readFile(this, files[i], opts);
        }
        return this;
      }

      if (Array.isArray(key) && arguments.length === 1) {
        key.forEach(function(val) {
          self.data(val);
        });
        return this;
      }

      if (typeof key === 'string') {
        opts = utils.extend({}, config, this.options);
        if (utils.isOptions(last)) {
          opts = utils.extend({}, opts, args.pop());
        }

        if (args.length === 1) {
          var res = readFile(this, key, opts);
          if (res === null) {
            if (/[\\\/]/.test(key)) {
              throw new Error('Failed to read: ' + key);
            }

            return data.get(key);
          }
          return this;
        }

        // if value is an object, merge it onto `key`
        if (utils.isObject(value)) {
          data.merge(key, value);
          return this;
        }

        if (union) {
          data.union.apply(data, arguments);
          return this;
        }

        data.set(key, value);
        return this;
      }
      return this;
    });

    /**
     * Expose all `Data` properties on `app.data`
     */

    this.data.__proto__ = data;

    /**
     * Read and parse a data file, and merge the resulting
     * object onto `app[prop]`
     */

    function readFile(app, fp, options) {
      var fns = utils.matchLoaders(app.dataLoaders, fp);
      if (!fns) return null;

      var opts = utils.extend({}, app.options, options);
      var val = utils.read.sync(fp);
      if (val === null) return null;

      var len = fns.length, i = -1;
      while (++i < len) {
        var fn = fns[i];
        val = fn.call(app, val, fp);
      }

      if (opts.namespace) {
        app.data(utils.namespace(fp, val, opts));
      } else {
        app.data(val);
      }
      return val;
    }

    return baseData;
  };
};

/**
 * Expose `utils`
 */

module.exports.utils = utils;

/**
 * Create an instance of `Data` with the given `cache` object
 * and `options`.
 *
 * @param {Object} `cache`
 * @param {Object} `options`
 */

function Data(cache, options) {
  Cache.call(this);
  this.options = options || {};
  this.cache = cache || {};
}

util.inherits(Data, Cache);

/**
 * Shallow extend an object onto `app.cache.data`.
 *
 * ```js
 * app.data({a: {b: {c: 'd'}}});
 * app.data.extend('a.b', {x: 'y'});
 * console.log(app.get('a.b'));
 * //=> {c: 'd', x: 'y'}
 * ```
 * @name .data.extend
 * @param {String|Object} `key` Property name or object to extend onto `app.cache.data`. Dot-notation may be used for extending nested properties.
 * @param {Object} `value` The object to extend onto `app.cache.data`
 * @return {Object} returns the instance for chaining
 * @api public
 */

Data.prototype.extend = function(key, val) {
  if (typeof key === 'string' && utils.isObject(val)) {
    var current = this.get(key);
    this.set(key, utils.extend({}, current, val));
    return this;
  }

  // key is either an array or object
  var args = utils.flatten([].slice.call(arguments));
  var len = args.length;
  var idx = -1;
  while (++idx < len) {
    utils.extend(this.cache, args[idx]);
  }
  return this;
};

/**
 * Deeply merge an object onto `app.cache.data`.
 *
 * ```js
 * app.data({a: {b: {c: {d: {e: 'f'}}}}});
 * app.data.merge('a.b', {c: {d: {g: 'h'}}});
 * console.log(app.get('a.b'));
 * //=> {c: {d: {e: 'f', g: 'h'}}}
 * ```
 * @name .data.merge
 * @param {String|Object} `key` Property name or object to merge onto `app.cache.data`. Dot-notation may be used for merging nested properties.
 * @param {Object} `value` The object to merge onto `app.cache.data`
 * @return {Object} returns the instance for chaining
 * @api public
 */

Data.prototype.merge = function(key, val) {
  if (typeof key === 'string' && utils.isObject(val)) {
    utils.mergeValue(this.cache, key, val);
    return this;
  }

  // key is either an array or object
  var args = utils.flatten([].slice.call(arguments));
  var len = args.length;
  var idx = -1;
  while (++idx < len) {
    this.cache = utils.merge(this.cache, args[idx]);
  }
  return this;
};

/**
 * Union the given value onto a new or existing array value on `app.cache.data`.
 *
 * ```js
 * app.data({a: {b: ['c', 'd']}});
 * app.data.union('a.b', ['e', 'f']}});
 * console.log(app.get('a.b'));
 * //=> ['c', 'd', 'e', 'f']
 * ```
 * @name .data.union
 * @param {String} `key` Property name. Dot-notation may be used for nested properties.
 * @param {Object} `array` The array to add or union on `app.cache.data`
 * @return {Object} returns the instance for chaining
 * @api public
 */

Data.prototype.union = function(key, arr) {
  utils.unionValue(this.cache, key, arr);
  return this;
};

/**
 * Set the given value onto `app.cache.data`.
 *
 * ```js
 * app.data.set('a.b', ['c', 'd']}});
 * console.log(app.get('a'));
 * //=> {b: ['c', 'd']}
 * ```
 * @name .data.set
 * @param {String|Object} `key` Property name or object to merge onto `app.cache.data`. Dot-notation may be used for nested properties.
 * @param {any} `val` The value to set on `app.cache.data`
 * @return {Object} returns the instance for chaining
 * @api public
 */

Data.prototype.set = function(key, val) {
  if (utils.isObject(key)) {
    return this.merge.apply(this, arguments);
  }
  utils.set(this.cache, key, val);
  return this;
};

/**
 * Get the value of `key` from `app.cache.data`. Dot-notation may be used for getting
 * nested properties.
 *
 * ```js
 * app.data({a: {b: {c: 'd'}}});
 * console.log(app.get('a.b'));
 * //=> {c: 'd'}
 * ```
 * @name .data.get
 * @param {String} `key` The name of the property to get.
 * @return {any} Returns the value of `key`
 * @api public
 */

Data.prototype.get = function(key) {
  return utils.get(this.cache, key);
};
