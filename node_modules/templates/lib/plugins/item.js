'use strict';

var utils = require('../utils');

/**
 * Adds a method to `app` for creating an instance of `Item` using the given `methodName`.
 *
 * ```js
 * app.use(item('view', 'View'));
 * ```
 * @param {String} `methodName`
 * @param {String} `CtorName` the constructor name to show when the item is inspected.
 * @return {Function}
 * @api public
 */

module.exports = function(methodName, CtorName, config) {
  config = config || {};

  return function(app) {
    if (typeof app[methodName] === 'function') return;

    /**
     * Returns a new item, using the `Item` class currently defined on the instance.
     *
     * ```js
     * var view = app.view('foo', {content: '...'});
     * // or
     * var view = app.view({path: 'foo', content: '...'});
     * ```
     * @name .item
     * @param {String|Object} `key` Item key or object
     * @param {Object} `value` If key is a string, value is the item object.
     * @return {Object} returns the `item` object
     * @api public
     */

    this.define(methodName, function(key, value, next) {
      if (typeof value === 'function') {
        next = value;
        value = null;
      }

      if (!value && typeof key === 'string') {
        value = { path: key };
      }

      if (utils.isObject(key) && key.path) {
        value = key;
        key = value.path;
      }

      if (typeof value === 'string') {
        value = { content: value };
      }

      // ensure value is an object (and not a function since `isObject` allows functions)
      if (!utils.isObject(value) || typeof value === 'function') {
        throw new TypeError('expected value to be an object.');
      }

      // set path before creating a new `Item`
      if (typeof key === 'string' && typeof value.path === 'undefined') {
        value.path = key;
      }

      var Item = this.get(CtorName);
      var item = (!value.isItem && !value.isView && !(value instanceof Item))
        ? new Item(value)
        : value;

      if (typeof item.is === 'function') {
        item.is(CtorName);
      }

      // ensure commonly needed objects are set on `item`, in case
      // a custom `Item` or `View` constructor was used
      if (!item.options) item.options = value.options || {};
      if (!item.locals) item.locals = value.locals || {};
      if (!item.data) item.data = value.data || {};

      // get renameKey fn if defined on item opts
      var renameKey = this.renameKey;
      if (item.options && typeof item.options.renameKey === 'function') {
        renameKey = item.options.renameKey;
      }

      item.key = renameKey.call(this, item.key || key, item);
      if (typeof next === 'function') {
        item.next = next;
      } else {
        item.next = function() {};
      }

      // get the collection name (singular form)
      var collectionName = item.options.inflection || this.options.inflection;
      item.options.collection = item.options.collection || this.options.plural;
      item.options.inflection = collectionName;

      // prime the object to use for caching locals and compiled functions
      utils.define(item, 'engineStack', item.engineStack || {});
      utils.define(item, 'localsStack', item.localsStack || []);

      // emit the item, collection name, and collection instance (`app.on('view', ...)`)
      if (config.emit !== false && this.hasListeners(methodName)) {
        this.emit(methodName, item, item.options.collection, this);
      }

      // if the instance is a top-level instance of templates (`isApp`), run plugins
      // on `item`, otherwise this is handled by collections
      if (app.isApp) {
        app.run(item);
      }

      return item;
    });
  };
};
