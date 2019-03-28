'use strict';

var utils = require('../utils');

module.exports = function(proto) {

  /**
   * Find a view by `name`, optionally passing a `collection` to limit
   * the search. If no collection is passed all `renderable` collections
   * will be searched.
   *
   * ```js
   * var page = app.find('my-page.hbs');
   *
   * // optionally pass a collection name as the second argument
   * var page = app.find('my-page.hbs', 'pages');
   * ```
   * @name .find
   * @param {String} `name` The name/key of the view to find
   * @param {String} `colleciton` Optionally pass a collection name (e.g. pages)
   * @return {Object|undefined} Returns the view if found, or `undefined` if not.
   * @api public
   */

  proto.find = function(name, collection) {
    if (typeof name !== 'string') {
      throw new TypeError('expected name to be a string.');
    }

    if (typeof collection === 'string') {
      if (this.views.hasOwnProperty(collection)) {
        return this[collection].getView(name);
      }
      throw new Error('collection ' + collection + ' does not exist');
    }

    for (var key in this.views) {
      var view = this[key].getView(name);
      if (view) {
        return view;
      }
    }
  };

  /**
   * Get view `key` from the specified `collection`.
   *
   * ```js
   * var view = app.getView('pages', 'a/b/c.hbs');
   *
   * // optionally pass a `renameKey` function to modify the lookup
   * var view = app.getView('pages', 'a/b/c.hbs', function(fp) {
   *   return path.basename(fp);
   * });
   * ```
   * @name .getView
   * @param {String} `collection` Collection name, e.g. `pages`
   * @param {String} `key` Template name
   * @param {Function} `fn` Optionally pass a `renameKey` function
   * @return {Object}
   * @api public
   */

  proto.getView = function(name, key, fn) {
    if (!this.views.hasOwnProperty(name)) {
      name = this.inflections[name];
    }
    var collection = this[name];
    var view = collection.views[key] || collection.getView(key);
    if (view) {
      return view;
    }
    // use renameKey function
    if (typeof fn === 'function') {
      key = fn.call(this, key);
    } else {
      key = this.renameKey(key);
    }
    view = collection.views[key] || collection.getView(key);
    if (view) {
      return view;
    }
  };

  /**
   * Get all views from a `collection` using the collection's
   * singular or plural name.
   *
   * ```js
   * var pages = app.getViews('pages');
   * //=> { pages: {'home.hbs': { ... }}
   *
   * var posts = app.getViews('posts');
   * //=> { posts: {'2015-10-10.md': { ... }}
   * ```
   *
   * @name .getViews
   * @param {String} `name` The collection name, e.g. `pages` or `page`
   * @return {Object}
   * @api public
   */

  proto.getViews = function(name) {
    var orig = name;
    if (utils.isObject(name)) return name;
    if (!this.views.hasOwnProperty(name)) {
      name = this.inflections[name];
    }
    if (!this.views.hasOwnProperty(name)) {
      throw new Error('getViews cannot find collection: ' + orig);
    }
    return this.views[name];
  };
};
