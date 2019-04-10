/*!
 * pkg-store <https://github.com/jonschlinkert/pkg-store>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var fs = require('fs');
var path = require('path');
var util = require('util');
var cache = require('cache-base');
var Cache = cache.namespace('data');
var utils = require('./utils');

/**
 * Initialize a new `Pkg` store at the given `cwd` with
 * the specified `options`.
 *
 * ```js
 * var pkg = require('pkg-store')(process.cwd());
 *
 * console.log(pkg.path);
 * //=> '~/your-project/package.json'
 *
 * console.log(pkg.data);
 * //=> {name: 'your-project', ...}
 * ```
 *
 * @param  {String} `cwd` Directory of the package.json to read.
 * @param  {Object} `options` Options to pass to [expand-pkg][] and [normalize-pkg][].
 * @api public
 */

function Pkg(cwd, options) {
  if (!(this instanceof Pkg)) {
    return new Pkg(cwd, options);
  }

  Cache.call(this);

  if (utils.isObject(cwd)) {
    options = cwd;
    cwd = null;
  }

  this.options = options || {};
  cwd = this.options.cwd || cwd || process.cwd();
  this.path = this.options.path || path.resolve(cwd, 'package.json');
  var data;

  Object.defineProperty(this, 'data', {
    configurable: true,
    enumerable: true,
    set: function(val) {
      data = val;
    },
    get: function() {
      return data || (data = this.read());
    }
  });
}

/**
 * Inherit `cache-base`
 */

util.inherits(Pkg, Cache);

/**
 * Concatenate the given val and uniquify array `key`.
 *
 * ```js
 * pkg.union('keywords', ['foo', 'bar', 'baz']);
 * ```
 * @param {String} `key`
 * @param {String} `val` Item or items to add to the array.
 * @return {Object} Returns the instance for chaining.
 * @api public
 */

Pkg.prototype.union = function(key, val) {
  utils.union(this.data, key, val);
  return this;
};

/**
 * Write the `pkg.data` object to `pkg.path` on the file system.
 *
 * ```js
 * pkg.save();
 * ```
 * @api public
 */

Pkg.prototype.save = function() {
  utils.writeJson.sync(this.path, this.data);
};

/**
 * Reads `pkg.path` from the file system and returns an object.
 *
 * ```js
 * var data = pkg.read();
 * ```
 * @api public
 */

Pkg.prototype.read = function() {
  if (utils.exists(this.path)) {
    return JSON.parse(fs.readFileSync(this.path, 'utf8'));
  }
  return {};
};

/**
 * Expoe `Pkg`
 */

module.exports = Pkg;
