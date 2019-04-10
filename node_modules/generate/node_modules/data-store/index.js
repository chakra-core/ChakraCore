'use strict';

/**
 * Module dependencies
 */

var path = require('path');
var util = require('util');
var base = require('cache-base');
var Base = base.namespace('cache');
var debug = require('debug')('data-store');
var proto = Base.prototype;
var utils = require('./utils');

/**
 * Expose `Store`
 */

module.exports = Store;

/**
 * Initialize a new `Store` with the given `name`
 * and `options`.
 *
 * ```js
 * var store = require('data-store')('abc');
 * //=> '~/data-store/a.json'
 *
 * var store = require('data-store')('abc', {
 *   cwd: 'test/fixtures'
 * });
 * //=> './test/fixtures/abc.json'
 * ```
 *
 * @param  {String} `name` Store name to use for the basename of the `.json` file.
 * @param  {Object} `options`
 * @param {String} `options.cwd` Current working directory for storage. If not defined, the user home directory is used, based on OS. This is the only option currently, other may be added in the future.
 * @param {Number} `options.indent` Number passed to `JSON.stringify` when saving the data. Defaults to `2` if `null` or `undefined`
 * @api public
 */

function Store(name, options) {
  if (!(this instanceof Store)) {
    return new Store(name, options);
  }

  if (typeof name !== 'string') {
    options = name;
    name = null;
  }

  Base.call(this);
  this.isStore = true;
  this.options = options || {};
  this.initStore(name);
}

/**
 * Inherit `Base`
 */

util.inherits(Store, Base);

/**
 * Initialize store defaults
 */

Store.prototype.initStore = function(name) {
  this.name = name || utils.project(process.cwd());
  this.cwd = utils.resolve(this.options.cwd || '~/.data-store');
  this.path = this.options.path || path.resolve(this.cwd, this.name + '.json');
  this.relative = path.relative(process.cwd(), this.path);

  debug('Initializing store <%s>', this.path);

  this.data = this.readFile(this.path);
  this.define('cache', utils.clone(this.data));
  this.on('set', function() {
    this.save();
  }.bind(this));
};

/**
 * Create a namespaced "sub-store" that persists data to its file
 * in a sub-folder of the same directory as the "parent" store.
 *
 * ```js
 * store.create('foo');
 * store.foo.set('a', 'b');
 * console.log(store.foo.get('a'));
 * //=> 'b'
 * ```
 * @param {String} `name` The name of the sub-store.
 * @param {Object} `options`
 * @return {Object} Returns the sub-store instance.
 * @api public
 */

Store.prototype.create = function(name, options) {
  if (utils.isStore(this, name)) {
    return this[name];
  }
  utils.validateName(this, name);

  var self = this;
  var cwd = path.join(path.dirname(this.path), this.name);
  var substore = new Store(name, { cwd: cwd });
  this[name] = substore;

  substore.on('set', function(key, val) {
    self.set([name, key], val);
  });

  return substore;
};

/**
 * Assign `value` to `key` and save to disk. Can be
 * a key-value pair or an object.
 *
 * ```js
 * // key, value
 * store.set('a', 'b');
 * //=> {a: 'b'}
 *
 * // extend the store with an object
 * store.set({a: 'b'});
 * //=> {a: 'b'}
 *
 * // extend the the given value
 * store.set('a', {b: 'c'});
 * store.set('a', {d: 'e'}, true);
 * //=> {a: {b 'c', d: 'e'}}
 *
 * // overwrite the the given value
 * store.set('a', {b: 'c'});
 * store.set('a', {d: 'e'});
 * //=> {d: 'e'}
 * ```
 * @name .set
 * @param {String} `key`
 * @param {any} `val` The value to save to `key`. Must be a valid JSON type: String, Number, Array or Object.
 * @return {Object} `Store` for chaining
 * @api public
 */

/**
 * Add or append an array of unique values to the given `key`.
 *
 * ```js
 * store.union('a', ['a']);
 * store.union('a', ['b']);
 * store.union('a', ['c']);
 * store.get('a');
 * //=> ['a', 'b', 'c']
 * ```
 *
 * @param  {String} `key`
 * @return {any} The array to add or append for `key`.
 * @api public
 */

Store.prototype.union = function(key, val) {
  utils.union(this.cache, key, val);
  this.emit('union', key, val);
  this.save();
  return this;
};

/**
 * Get the stored `value` of `key`, or return the entire store
 * if no `key` is defined.
 *
 * ```js
 * store.set('a', {b: 'c'});
 * store.get('a');
 * //=> {b: 'c'}
 *
 * store.get();
 * //=> {b: 'c'}
 * ```
 *
 * @name .get
 * @param  {String} `key`
 * @return {any} The value to store for `key`.
 * @api public
 */

/**
 * Returns `true` if the specified `key` has truthy value.
 *
 * ```js
 * store.set('a', 'b');
 * store.set('c', null);
 * store.has('a'); //=> true
 * store.has('c'); //=> false
 * store.has('d'); //=> false
 * ```
 * @name .has
 * @param  {String} `key`
 * @return {Boolean} Returns true if `key` has
 * @api public
 */

/**
 * Returns `true` if the specified `key` exists.
 *
 * ```js
 * store.set('a', 'b');
 * store.set('b', false);
 * store.set('c', null);
 * store.set('d', true);
 *
 * store.hasOwn('a'); //=> true
 * store.hasOwn('b'); //=> true
 * store.hasOwn('c'); //=> true
 * store.hasOwn('d'); //=> true
 * store.hasOwn('foo'); //=> false
 * ```
 *
 * @param  {String} `key`
 * @return {Boolean} Returns true if `key` exists
 * @api public
 */

Store.prototype.hasOwn = function(key) {
  var val;
  if (key.indexOf('.') === -1) {
    val = this.cache.hasOwnProperty(key);
  } else {
    val = utils.hasOwn(this.cache, key);
  }
  return val;
};

/**
 * Persist the store to disk.
 *
 * ```js
 * store.save();
 * ```
 * @param {String} `dest` Optionally define an alternate destination file path.
 * @api public
 */

Store.prototype.save = function(dest) {
  this.data = this.cache;
  writeJson(dest || this.path, this.cache, this.options.indent);
  return this;
};

/**
 * Clear in-memory cache.
 *
 * ```js
 * store.clear();
 * ```
 * @api public
 */

Store.prototype.clear = function() {
  this.cache = {};
  this.data = {};
  return this;
};

/**
 * Delete `keys` from the store, or delete the entire store
 * if no keys are passed. A `del` event is also emitted for each key
 * deleted.
 *
 * **Note that to delete the entire store you must pass `{force: true}`**
 *
 * ```js
 * store.del();
 *
 * // to delete paths outside cwd
 * store.del({force: true});
 * ```
 *
 * @param {String|Array|Object} `keys` Keys to remove, or options.
 * @param {Object} `options`
 * @api public
 */

Store.prototype.del = function(keys, options) {
  var isArray = Array.isArray(keys);
  if (typeof keys === 'string' || isArray) {
    keys = utils.arrayify(keys);
  } else {
    options = keys;
    keys = null;
  }

  options = options || {};

  if (keys) {
    keys.forEach(function(key) {
      proto.del.call(this, key);
    }.bind(this));
    this.save();
    return this;
  }

  if (options.force !== true) {
    throw new Error('options.force is required to delete the entire cache.');
  }

  keys = Object.keys(this.cache);
  this.clear();

  // if no keys are passed, delete the entire store
  utils.del.sync(this.path, options);
  keys.forEach(function(key) {
    this.emit('del', key);
  }.bind(this));
  return this;
};

/**
 * Returns an array of all Store properties.
 */

utils.define(Store.prototype, 'keys', {
  configurable: true,
  enumerable: true,
  set: function(keys) {
    utils.define(this, 'keys', keys);
  },
  get: function fn() {
    if (fn.keys) return fn.keys;
    fn.keys = [];
    for (var key in this) fn.keys.push(key);
    return fn.keys;
  }
});

/**
 * Define a non-enumerable property on the instance.
 *
 * @param {String} `key`
 * @param {any} `value`
 * @return {Object} Returns the instance for chaining.
 * @api public
 */

Store.prototype.define = function(key, value) {
  utils.define(this, key, value);
  return this;
};

/**
 * Read JSON files.
 *
 * @param {String} `fp`
 * @return {Object}
 */

Store.prototype.readFile = function(filepath) {
  try {
    var str = utils.fs.readFileSync(path.resolve(filepath), 'utf8');
    this.loadedConfig = true;
    return JSON.parse(str);
  } catch (err) {}
  this.loadedConfig = false;
  return {};
};

/**
 * Synchronously write files to disk, also creating any
 * intermediary directories if they don't exist.
 *
 * @param {String} `dest`
 * @param {String} `str`
 * @param {Number} `indent` Indent passed to JSON.stringify (default 2)
 */

function writeJson(dest, str, indent) {
  if (typeof indent === 'undefined' || indent === null) {
    indent = 2;
  }
  var dir = path.dirname(dest);
  try {
    if (!utils.fs.existsSync(dir)) {
      utils.mkdirp.sync(dir);
    }
    utils.fs.writeFileSync(dest, JSON.stringify(str, null, indent));
  } catch (err) {
    err.origin = __filename;
    err.reason = 'data-store cannot write to: ' + dest;
    throw new Error(err);
  }
}
