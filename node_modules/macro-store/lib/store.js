/*!
 * macro-store (https://github.com/doowb/macro-store)
 *
 * Copyright (c) 2016, Brian Woodward.
 * Licensed under the MIT License.
 */

'use strict';

var debug = require('debug')('macro-store');
var utils = require('./utils');

/**
 * Initialize a new `Store` with the given `options`.
 *
 * ```js
 * var macroStore = new Store();
 * //=> '~/data-store/macros.json'
 *
 * var macroStore = new Store({name: 'abc'});
 * //=> '~/data-store/abc.json'
 * ```
 *
 * @param  {Object} `options`
 * @param {String} `options.name` Name of the json file to use for storing macros. Defaults to 'macros'
 * @param {Object} `options.store` Instance of [data-store][] to use. Allows complete control over where the store is located.
 * @api public
 */

function Store(options) {
  if (!(this instanceof Store)) {
    return new Store(options);
  }

  if (typeof options === 'string') {
    options = { name: options };
  }
  options = options || {};
  var name = options.name || 'macros';
  this.store = options.store || new utils.Store(name);
}

/**
 * Set a macro in the store.
 *
 * ```js
 * macroStore.set('foo', ['foo', 'bar', 'baz']);
 * ```
 *
 * @param {String} `key` Name of the macro to set.
 * @param {Array} `arr` Array of strings that the macro will resolve to.
 * @returns {Object} `this` for chaining
 * @api public
 */

Store.prototype.set = function(key, arr) {
  debug('macro-store.set', key, arr);
  this.store.set(['macro', key], arr);
  return this;
};

/**
 * Get a macro from the store.
 *
 * ```js
 * var tasks = macroStore.get('foo');
 * //=> ['foo', 'bar', 'baz']
 *
 * // returns input name when macro is not in the store
 * var tasks = macroStore.get('bar');
 * //=> 'bar'
 * ```
 * @param  {String} `name` Name of macro to get.
 * @return {String|Array} Array of tasks to get from a stored macro, or the original name when a stored macro does not exist.
 * @api public
 */

Store.prototype.get = function(name) {
  debug('macro-store.get', name);
  var macro = utils.arrayify(this.store.get(['macro', name]));
  return macro.length ? macro : name;
};

/**
 * Remove a macro from the store.
 *
 * ```js
 * macroStore.del('foo');
 * ```
 * @param  {String|Array} `name` Name of a macro or array of macros to remove.
 * @return {Object} `this` for chaining
 * @api public
 */

Store.prototype.del = function(arr) {
  debug('macro-store.del', arr);
  arr = utils.arrayify(arr);
  if (arr.length === 0) {
    this.store.del('macro');
    return;
  }

  this.store.del(arr.map(function(name) {
    return `macro.${name}`;
  }));
  return this;
};

/**
 * Expose `Store`
 */

module.exports = Store;
