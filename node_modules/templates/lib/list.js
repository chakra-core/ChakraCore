'use strict';

var Base = require('base');
var debug = require('debug')('base:templates:list');
var plugin = require('./plugins');
var Group = require('./group');
var utils = require('./utils');

/**
 * Expose `List`
 */

module.exports = exports = List;

/**
 * Create an instance of `List` with the given `options`.
 * Lists differ from collections in that items are stored
 * as an array, allowing items to be paginated, sorted,
 * and grouped.
 *
 * ```js
 * var list = new List();
 * list.addItem('foo', {content: 'bar'});
 * ```
 * @param {Object} `options`
 * @api public
 */

function List(options) {
  if (!(this instanceof List)) {
    return new List(options);
  }

  Base.call(this);

  this.is('list');
  this.define('isCollection', true);
  this.use(utils.option());
  this.use(utils.plugin());
  this.init(options || {});
}

/**
 * Inherit `Base` and load static plugins
 */

plugin.static(Base, List, 'List');

/**
 * Initalize `List` defaults
 */

List.prototype.init = function(options) {
  debug('initalizing');

  // add constructors to the instance
  this.define('Item', options.Item || List.Item);
  this.define('View', options.View || List.View);

  // decorate the instance
  this.use(plugin.init);
  this.use(plugin.renameKey());
  this.use(plugin.context);
  this.use(utils.engines());
  this.use(utils.helpers());
  this.use(utils.routes());
  this.use(plugin.item('item', 'Item', {emit: false}));
  this.use(plugin.item('view', 'View', {emit: false}));

  // decorate `isItem` and `isView`, in case custom class is passed
  plugin.is(this.Item);
  plugin.is(this.View);

  this.queue = [];
  this.items = [];
  this.keys = [];

  // add some "list" methods to "list.items" to simplify usage in helpers
  decorate(this, 'paginate');
  decorate(this, 'filter', 'items');
  decorate(this, 'groupBy', 'items');
  decorate(this, 'sortBy', 'items');

  // if an instance of `List` of `Views` is passed, load it now
  if (Array.isArray(options)) {
    this.options = {};
    this.addList(options);

  } else if (options.isList) {
    this.options = options.options;
    this.addList(options.items);

  } else if (options.isCollection) {
    this.options = options.options;
    this.addItems(options.views);

  } else {
    this.options = options;
  }
};

/**
 * Add an item to `list.items`. This is identical to [setItem](#setItem)
 * except `addItem` returns the `item`, add `setItem` returns the instance
 * of `List`.
 *
 * ```js
 * collection.addItem('foo', {content: 'bar'});
 * ```
 *
 * @param {String|Object} `key` Item key or object
 * @param {Object} `value` If key is a string, value is the item object.
 * @developer The `item` method is decorated onto the collection using the `item` plugin
 * @return {Object} returns the `item` instance.
 * @api public
 */

List.prototype.addItem = function(key, value) {
  var item = this.item(key, value);
  utils.setInstanceNames(item, 'Item');

  debug('adding item "%s"', item.key);

  if (this.options.pager === true) {
    List.addPager(item, this.items);
  }

  this.keys.push(item.key);
  if (typeof item.use === 'function') {
    this.run(item);
  }

  this.extendItem(item);
  this.emit('load', item, this);
  this.emit('item', item, this);

  this.items.push(item);
  return item;
};

/**
 * Add an item to `list.items`. This is identical to [addItem](#addItem)
 * except `addItem` returns the `item`, add `setItem` returns the instance
 * of `List`.
 *
 * ```js
 * var items = new Items(...);
 * items.setItem('a.html', {path: 'a.html', contents: '...'});
 * ```
 * @param {String} `key`
 * @param {Object} `value`
 * @return {Object} Returns the instance of `List` to support chaining.
 * @api public
 */

List.prototype.setItem = function(/*key, value*/) {
  this.addItem.apply(this, arguments);
  return this;
};

/**
 * Load multiple items onto the collection.
 *
 * ```js
 * collection.addItems({
 *   'a.html': {content: '...'},
 *   'b.html': {content: '...'},
 *   'c.html': {content: '...'}
 * });
 * ```
 * @param {Object|Array} `items`
 * @return {Object} returns the instance for chaining
 * @api public
 */

List.prototype.addItems = function(items) {
  if (Array.isArray(items)) {
    return this.addList.apply(this, arguments);
  }

  this.emit('addItems', items);
  if (this.loaded) {
    this.loaded = false;
    return this;
  }

  this.visit('addItem', items);
  return this;
};

/**
 * Load an array of items or the items from another instance of `List`.
 *
 * ```js
 * var foo = new List(...);
 * var bar = new List(...);
 * bar.addList(foo);
 * ```
 * @param {Array} `items` or an instance of `List`
 * @param {Function} `fn` Optional sync callback function that is called on each item.
 * @return {Object} returns the List instance for chaining
 * @api public
 */

List.prototype.addList = function(list, fn) {
  this.emit.call(this, 'addList', list);
  if (this.loaded) {
    this.loaded = false;
    return this;
  }

  if (!Array.isArray(list)) {
    throw new TypeError('expected list to be an array.');
  }

  var len = list.length, i = -1;
  while (++i < len) {
    var item = list[i];
    if (typeof fn === 'function') {
      item = fn(item) || item;
    }
    this.addItem(item.path, item);
  }
  return this;
};

/**
 * Return true if the list has the given item (name).
 *
 * ```js
 * list.addItem('foo.html', {content: '...'});
 * list.hasItem('foo.html');
 * //=> true
 * ```
 * @param {String} `key`
 * @return {Object}
 * @api public
 */

List.prototype.hasItem = function(key) {
  return this.getIndex(key) !== -1;
};

/**
 * Get a the index of a specific item from the list by `key`.
 *
 * ```js
 * list.getIndex('foo.html');
 * //=> 1
 * ```
 * @param {String} `key`
 * @return {Object}
 * @api public
 */

List.prototype.getIndex = function(key) {
  if (typeof key === 'undefined') {
    throw new Error('expected a string or instance of Item');
  }

  if (List.isItem(key)) {
    key = key.key;
  }

  var idx = this.keys.indexOf(key);
  if (idx !== -1) {
    return idx;
  }

  idx = this.keys.indexOf(this.renameKey(key));
  if (idx !== -1) {
    return idx;
  }

  var items = this.items;
  var len = items.length;

  while (len--) {
    var item = items[len];
    var prop = this.renameKey(key, item);
    if (utils.matchFile(prop, item)) {
      return len;
    }
  }
  return -1;
};

/**
 * Get a specific item from the list by `key`.
 *
 * ```js
 * list.getItem('foo.html');
 * //=> '<Item <foo.html>>'
 * ```
 * @param {String} `key` The item name/key.
 * @return {Object}
 * @api public
 */

List.prototype.getItem = function(key) {
  var idx = this.getIndex(key);
  if (idx !== -1) {
    return this.items[idx];
  }
};

/**
 * Proxy for `getItem`
 *
 * ```js
 * list.getItem('foo.html');
 * //=> '<Item "foo.html" <buffer e2 e2 e2>>'
 * ```
 * @param {String} `key` Pass the key of the `item` to get.
 * @return {Object}
 * @api public
 */

List.prototype.getView = function() {
  return this.getItem.apply(this, arguments);
};

/**
 * Remove an item from the list.
 *
 * ```js
 * list.deleteItem('a.html');
 * ```
 * @param {Object|String} `key` Pass an `item` instance (object) or `item.key` (string).
 * @api public
 */

List.prototype.deleteItem = function(item) {
  var idx = this.getIndex(item);
  if (idx !== -1) {
    this.items.splice(idx, 1);
    this.keys.splice(idx, 1);
  }
  return this;
};

/**
 * Remove one or more items from the list.
 *
 * ```js
 * list.deleteItems(['a.html', 'b.html']);
 * ```
 * @param {Object|String|Array} `items` List of items to remove.
 * @api public
 */

List.prototype.deleteItems = function(items) {
  if (!Array.isArray(items)) {
    return this.deleteItem.apply(this, arguments);
  }
  for (var i = 0; i < items.length; i++) {
    this.deleteItem(items[i]);
  }
  return this.items;
};

/**
 * Decorate each item on the list with additional methods
 * and properties. This provides a way of easily overriding
 * defaults.
 *
 * @param {Object} `item`
 * @return {Object} Instance of item for chaining
 * @api public
 */

List.prototype.extendItem = function(item) {
  plugin.view(this, item);
  return this;
};

/**
 * Filters list `items` using the given `fn` and returns
 * a new array.
 *
 * ```js
 * var items = list.filter(function(item) {
 *   return item.data.title.toLowerCase() !== 'home';
 * });
 * ```
 * @return {Object} Returns a filtered array of items.
 * @api public
 */

List.prototype.filter = function(fn) {
  var list = new List(this.options);
  list.addItems(this.items.slice().filter(fn));
  return list;
};

/**
 * Sort all list `items` using the given property,
 * properties or compare functions. See [array-sort][]
 * for the full range of available features and options.
 *
 * ```js
 * var list = new List();
 * list.addItems(...);
 * var result = list.sortBy('data.date');
 * //=> new sorted list
 * ```
 * @return {Object} Returns a new `List` instance with sorted items.
 * @api public
 */

List.prototype.sortBy = function(items) {
  var args = [].slice.call(arguments);
  var last = args[args.length - 1];
  var opts = this.options.sort || {};

  // extend `list.options.sort` global options with local options
  if (last && typeof last === 'object' && !Array.isArray(last)) {
    opts = utils.extend({}, opts, args.pop());
  }

  if (!Array.isArray(items)) {
    args.unshift(this.items.slice());
  }

  // create the args to pass to array-sort
  args.push(opts);

  // sort the `items` array, then sort `keys`
  items = utils.sortBy.apply(utils.sortBy, args);
  var list = new List(this.options);
  list.addItems(items);
  return list;
};

/**
 * Group all list `items` using the given property, properties or
 * compare functions. See [group-array][] for the full range of available
 * features and options.
 *
 * ```js
 * var list = new List();
 * list.addItems(...);
 * var groups = list.groupBy('data.date', 'data.slug');
 * ```
 * @return {Object} Returns the grouped items.
 * @api public
 */

List.prototype.groupBy = function(items) {
  var args = arguments;
  var fn = utils.groupBy;

  if (!Array.isArray(items)) {
    // Make `items` the first argument for paginationator
    args = [].slice.call(arguments);
    args.unshift(this.items.slice());
  }

  // group the `items` and return the result
  return new Group(fn.apply(fn, args));
};

/**
 * Paginate all `items` in the list with the given options,
 * See [paginationator][] for the full range of available
 * features and options.
 *
 * ```js
 * var list = new List(items);
 * var pages = list.paginate({limit: 5});
 * ```
 * @return {Object} Returns the paginated items.
 * @api public
 */

List.prototype.paginate = function(items, options) {
  var fn = utils.paginationator;
  var args = arguments;

  if (!Array.isArray(items)) {
    args = [].slice.call(arguments);
    // Make `items` the first argument for paginationator
    args.unshift(this.items.slice());
  }

  // paginate the `items` and return the pages
  items = fn.apply(fn, args);
  return items.pages;
};

/**
 * Resolve the layout to use for the given `item`
 *
 * @param {Object} `item`
 * @return {String} Returns the name of the layout to use.
 */

List.prototype.resolveLayout = function(item) {
  if (utils.isRenderable(item) && typeof item.layout === 'undefined') {
    return this.option('layout');
  }
  return item.layout;
};

/**
 * Add paging (`prev`/`next`) information to the
 * `data` object of an item.
 *
 * @param {Object} `item`
 * @param {Array} `items` instance items.
 */

List.addPager = function addPager(item, items) {
  var len = items.length;
  var prev = items[len - 1] || null;

  item.data.pager = {};
  utils.define(item.data.pager, 'isPager', true);
  utils.define(item.data.pager, 'current', item);

  item.data.pager.index = len;
  item.data.pager.isFirst = len === 0;
  item.data.pager.first = items[0] || item;
  item.data.pager.prev = prev;

  if (prev) {
    prev.data.pager.next = item;
  }

  Object.defineProperty(item.data.pager, 'isLast', {
    configurable: true,
    enumerable: true,
    get: function() {
      return this.index === items.length - 1;
    }
  });

  Object.defineProperty(item.data.pager, 'last', {
    configurable: true,
    enumerable: true,
    get: function() {
      return items[items.length - 1];
    }
  });
};

/**
 * Expose static properties
 */

utils.define(List, 'Item', require('vinyl-item'));
utils.define(List, 'View', require('vinyl-view'));

/**
 * Helper function for decorating methods onto the `list.items` array
 */

function decorate(list, method, prop) {
  utils.define(list.items, method, function() {
    var res = list[method].apply(list, arguments);
    return prop ? res[prop] : res;
  });
}
