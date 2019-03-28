'use strict';

var fs = require('fs');
var Base = require('base');
var debug = require('debug')('base:templates:views');
var plugin = require('./plugins');
var utils = require('./utils');
var List = require('./list');

/**
 * Expose `Views`
 */

module.exports = exports = Views;

/**
 * Create an instance of `Views` with the given `options`.
 *
 * ```js
 * var collection = new Views();
 * collection.addView('foo', {content: 'bar'});
 * ```
 * @param {Object} `options`
 * @api public
 */

function Views(options) {
  if (!(this instanceof Views)) {
    return new Views(options);
  }

  Base.call(this);

  this.is('views');
  this.define('isCollection', true);
  this.use(utils.option());
  this.use(utils.plugin());
  this.init(options || {});
}

/**
 * Inherit `Base` and load plugins
 */

plugin.static(Base, Views, 'Views');

/**
 * Initialize `Views` defaults
 */

/**
 * Expose static properties
 */

Views.prototype.init = function(opts) {
  debug('initializing');

  // add constructors to the instance
  this.define('Item', opts.Item || Views.Item);
  this.define('View', opts.View || Views.View);

  // decorate the instance
  this.use(plugin.init);
  this.use(plugin.renameKey());
  this.use(plugin.context);
  this.use(utils.engines());
  this.use(utils.helpers());
  this.use(utils.routes());
  this.use(plugin.item('view', 'View', {emit: false}));

  // setup listeners
  this.listen(this);
  this.views = {};

  // if an instance of `List` of `Views` is passed, load it now
  if (Array.isArray(opts) || opts.isList) {
    this.option(opts.options || {});
    this.addList(opts.items || opts);

  } else if (opts.isCollection) {
    this.option(opts.options || {});
    this.addViews(opts.views);

  } else {
    this.option(opts);
  }
};

/**
 * Built-in listeners
 */

Views.prototype.listen = function(collection) {
  // ensure that plugins are loaded onto views
  // created after the plugins are registered
  this.on('use', function(fn) {
    if (typeof fn !== 'function') return;
    for (var key in collection.views) {
      if (collection.views.hasOwnProperty(key)) {
        var view = collection.views[key];
        if (typeof view.use === 'function') {
          view.use(fn);
        }
      }
    }
  });
};

/**
 * Add a view to `collection.views`. This is identical to [addView](#addView)
 * except `setView` returns the collection instance, and `addView` returns
 * the item instance.
 *
 * ```js
 * collection.setView('foo', {content: 'bar'});
 *
 * // or, optionally async
 * collection.setView('foo', {content: 'bar'}, function(err, view) {
 *   // `err` errors from `onLoad` middleware
 *   // `view` the view object after `onLoad` middleware has run
 * });
 * ```
 *
 * @param {String|Object} `key` View key or object
 * @param {Object} `value` If key is a string, value is the view object.
 * @param {Function} `next` Optionally pass a callback function as the last argument to load the view asynchronously. This will also ensure that `.onLoad` middleware is executed asynchronously.
 * @developer This method is decorated onto the collection in the constructor using the `createView` utility method.
 * @return {Object} returns the `view` instance.
 * @api public
 */

Views.prototype.addView = function(key, value, next) {
  var view = this.view(key, value, next);
  debug('adding view "%s"', view.path);

  // set the `viewType` (partial, layout, or renderable)
  this.setType(view);

  // set the name to be used by the `inspect` method and emitter
  var name = this.options.inflection || 'view';
  utils.setInstanceNames(view, name);

  // run plugins on `view`
  if (typeof view.use === 'function') {
    this.run(view);
  }

  // emit that the view has been loaded
  this.emit('load', view, this);
  this.emit('view', view, this);
  this.emit(name, view, this);
  this.extendView(view);

  // set the view on `collection.views`
  this.views[view.key] = view;
  return view;
};

/**
 * Set a view on the collection. This is identical to [addView](#addView)
 * except `setView` does not emit an event for each view.
 *
 * ```js
 * collection.setView('foo', {content: 'bar'});
 * ```
 *
 * @param {String|Object} `key` View key or object
 * @param {Object} `value` If key is a string, value is the view object.
 * @developer This method is decorated onto the collection in the constructor using the `createView` utility method.
 * @return {Object} returns the `view` instance.
 * @api public
 */

Views.prototype.setView = function(/*key, value*/) {
  this.addView.apply(this, arguments);
  return this;
};

/**
 * Get view `name` from `collection.views`.
 *
 * ```js
 * collection.getView('a.html');
 * ```
 * @param {String} `key` Key of the view to get.
 * @param {Function} `fn` Optionally pass a function to modify the key.
 * @return {Object}
 * @api public
 */

Views.prototype.getView = function(name, options, fn) {
  if (typeof name !== 'string') {
    throw new TypeError('expected a string');
  }

  debug('getting view "%s"', name);
  if (typeof options === 'function') {
    fn = options;
    options = {};
  }

  var view = this.views[name] || this.views[this.renameKey(name)];
  if (view) return view;

  view = utils.getView(name, this.views, fn);
  if (view) return view;

  if (utils.fileExists(name)) {
    return this.addView(name, {
      contents: fs.readFileSync(name)
    });
  }
};

/**
 * Delete a view from collection `views`.
 *
 * ```js
 * views.deleteView('foo.html');
 * ```
 * @param {String} `key`
 * @return {Object} Returns the instance for chaining
 * @api public
 */

Views.prototype.deleteView = function(view) {
  if (typeof view === 'string') {
    view = this.getView(view);
  }
  debug('deleting view "%s"', view.key);
  delete this.views[view.key];
  return this;
};

/**
 * Load multiple views onto the collection.
 *
 * ```js
 * collection.addViews({
 *   'a.html': {content: '...'},
 *   'b.html': {content: '...'},
 *   'c.html': {content: '...'}
 * });
 * ```
 * @param {Object|Array} `views`
 * @return {Object} returns the `collection` object
 * @api public
 */

Views.prototype.addViews = function(views, view) {
  this.emit('addViews', views);
  if (utils.hasGlob(views)) {
    var name = '"' + this.options.plural + '"';
    throw new Error('glob patterns are not supported by the ' + name + ' collection');
  }
  if (Array.isArray(views)) {
    return this.addList.apply(this, arguments);
  }
  if (arguments.length > 1 && utils.isView(view)) {
    this.addView.apply(this, arguments);
    return this;
  }
  this.visit('addView', views);
  return this;
};

/**
 * Load an array of views onto the collection.
 *
 * ```js
 * collection.addList([
 *   {path: 'a.html', content: '...'},
 *   {path: 'b.html', content: '...'},
 *   {path: 'c.html', content: '...'}
 * ]);
 * ```
 * @param {Array} `list`
 * @return {Object} returns the `views` instance
 * @api public
 */

Views.prototype.addList = function(list, fn) {
  this.emit('addList', list);

  if (utils.hasGlob(list)) {
    var name = '"' + this.options.plural + '"';
    throw new Error('glob patterns are not supported by the ' + name + ' collection');
  }

  if (!Array.isArray(list)) {
    throw new TypeError('expected list to be an array.');
  }
  if (typeof fn !== 'function') {
    fn = utils.identity;
  }

  var len = list.length;
  var idx = -1;
  while (++idx < len) {
    this.addView(fn(list[idx]));
  }
  return this;
};

/**
 * Group all collection `views` by the given property,
 * properties or compare functions. See [group-array][]
 * for the full range of available features and options.
 *
 * ```js
 * var collection = new Collection();
 * collection.addViews(...);
 * var groups = collection.groupBy('data.date', 'data.slug');
 * ```
 * @return {Object} Returns an object of grouped views.
 * @api public
 */

Views.prototype.groupBy = function() {
  var list = new List(this);
  return list.groupBy.apply(list, arguments);
};

/**
 * Used for extending `view` with custom properties or methods.
 *
 * ```js
 * collection.extendView(view);
 * ```
 * @param {Object} `view`
 * @return {Object}
 */

Views.prototype.extendView = function(view) {
  return plugin.view(this, view, this.options);
};

/**
 * Return true if the collection belongs to the given
 * view `type`.
 *
 * ```js
 * collection.isType('partial');
 * ```
 * @param {String} `type` (`renderable`, `partial`, `layout`)
 * @api public
 */

Views.prototype.isType = function(type) {
  if (!this.options.viewType || !this.options.viewType.length) {
    this.viewType();
  }
  return this.options.viewType.indexOf(type) !== -1;
};

/**
 * Set view types for the collection.
 *
 * @param {String} `plural` e.g. `pages`
 * @param {Object} `options`
 */

Views.prototype.viewType = function(types) {
  this.options.viewType = utils.arrayify(this.options.viewType);
  types = utils.arrayify(types);

  var validTypes = ['partial', 'layout', 'renderable'];
  var len = types.length;
  var idx = -1;

  while (++idx < len) {
    var type = types[idx];
    if (validTypes.indexOf(type) === -1) {
      var msg = 'Invalid viewType: "' + type
        + '". viewTypes must be either: "partial", '
        + '"renderable", or "layout".';
      throw new Error(msg);
    }
    if (this.options.viewType.indexOf(type) === -1) {
      this.options.viewType.push(type);
    }
  }

  if (this.options.viewType.length === 0) {
    this.options.viewType.push('renderable');
  }
  return this.options.viewType;
};

/**
 * Alias for `viewType`
 *
 * @api public
 */

Views.prototype.viewTypes = function() {
  return this.viewType.apply(this, arguments);
};

/**
 * Update the `options.viewType` property on a view.
 *
 * @param {Object} `view` The view to update
 */

Views.prototype.setType = function(view) {
  view.options.viewType = utils.arrayify(view.options.viewType);
  var types = this.viewType();
  var len = types.length;

  while (len--) {
    var type = types[len];
    if (view.options.viewType.indexOf(type) === -1) {
      view.options.viewType.push(type);
    }
  }
  view.options.viewType.sort();
};

/**
 * Resolve the layout to use for the given `view`
 *
 * @param {Object} `view`
 * @return {String} Returns the name of the layout to use.
 */

Views.prototype.resolveLayout = function(view) {
  if (!utils.isPartial(view) && typeof view.layout === 'undefined') {
    return this.option('layout');
  }
  return view.layout;
};

/**
 * Expose static properties
 */

utils.define(Views, 'Item', require('vinyl-item'));
utils.define(Views, 'View', require('vinyl-view'));
