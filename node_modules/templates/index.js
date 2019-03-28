/*!
 * templates <https://github.com/jonschlinkert/templates>
 *
 * Copyright (c) 2015-2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var Base = require('base');
var debug = require('debug')('base:templates');
var helpers = require('./lib/helpers/');
var plugin = require('./lib/plugins/');
var utils = require('./lib/utils');
var lib = require('./lib/');

/**
 * Expose `Templates`
 */

module.exports = exports = Templates;

/**
 * This function is the main export of the templates module.
 * Initialize an instance of `templates` to create your
 * application.
 *
 * ```js
 * var templates = require('templates');
 * var app = templates();
 * ```
 * @param {Object} `options`
 * @api public
 */

function Templates(options) {
  if (!(this instanceof Templates)) {
    return new Templates(options);
  }

  Base.call(this, null, options);
  this.is('templates');
  this.define('isApp', true);
  this.use(utils.option());
  this.use(utils.plugin());
  this.initTemplates();
}

/**
 * Inherit `Base` and load static plugins
 */

plugin.static(Base, Templates, 'Templates');

/**
 * Initialize Templates
 */

Templates.prototype.initTemplates = function() {
  debug('initializing <%s>, called from <%s>', __filename, module.parent.id);
  Templates.emit('templates.preInit', this);

  this.items = {};
  this.views = {};
  this.inflections = {};

  // listen for options events
  this.listen(this);

  this.define('utils', utils);
  this.use(plugin.init);
  this.use(plugin.renameKey());
  this.use(plugin.context);
  this.use(plugin.lookup);
  this.use(utils.engines());
  this.use(utils.helpers());
  this.use(utils.routes());

  this.use(plugin.item('item', 'Item'));
  this.use(plugin.item('view', 'View'));

  for (var key in this.options.mixins) {
    this.mixin(key, this.options.mixins[key]);
  }

  // create an async `view` helper
  helpers.view(this);

  // expose constructors on the instance
  this.expose('Item');
  this.expose('View');
  this.expose('List');
  this.expose('Collection');
  this.expose('Group');
  this.expose('Views');

  Templates.setup(this, 'Templates');
  Templates.emit('templates.postInit', this);
};

/**
 * Expose constructors on app instance, allowing them to be
 * overridden by the user after Templates is instantiated.
 */

Templates.prototype.expose = function(name) {
  this.define(name, {
    configurable: true,
    enumerable: true,
    get: function() {
      return this.options[name] || Templates[name];
    }
  });
};

/**
 * Listen for events
 */

Templates.prototype.listen = function(app) {
  this.on('option', function(key, value) {
    utils.updateOptions(app, key, value);
  });

  // ensure that plugins are loaded onto collections
  // created after the plugins are registered
  this.on('use', function(fn, app) {
    if (!fn) return;
    for (var key in app.views) {
      if (app.views.hasOwnProperty(key)) {
        app[key].__proto__.use(fn);
      }
    }
  });
};

/**
 * Create a new list. See the [list docs](docs/lists.md) for more
 * information about lists.
 *
 * ```js
 * var list = app.list();
 * list.addItem('abc', {content: '...'});
 *
 * // or, create list from a collection
 * app.create('pages');
 * var list = app.list(app.pages);
 * ```
 * @param  {Object} `opts` List options
 * @return {Object} Returns the `list` instance for chaining.
 * @api public
 */

Templates.prototype.list = function(options) {
  options = options || {};
  var opts = {};

  if (options.isList || options.isViews) {
    opts = utils.merge({}, this.options, options.options);
  } else {
    opts = utils.merge({}, this.options, options);
  }

  var List = opts.List || this.get('List');
  var list = {};

  if (!options.isList) {
    opts.Item = opts.Item || this.get('Item');
    list = new List(opts);

    if (options.isViews) {
      list.addItems(options.views);
    }
  } else {
    list = options;
  }

  // customize list items
  this.extendList(list, opts);

  // emit the list
  this.emit('list', list, opts);
  return list;
};

/**
 * Create a new collection. Collections are decorated with
 * special methods for getting and setting items from the
 * collection. Note that, unlike the [create](#create) method,
 * collections created with `.collection()` are not cached.
 *
 * See the [collection docs](docs/collections.md) for more
 * information about collections.
 *
 * @param  {Object} `opts` Collection options
 * @return {Object} Returns the `collection` instance for chaining.
 * @api public
 */

Templates.prototype.collection = function(options, created) {
  options = options || {};
  var opts = {};

  if (options.isList || options.isCollection) {
    opts = utils.merge({}, this.options, options.options);
  } else {
    opts = utils.merge({}, this.options, options);
  }

  var Collection = opts.Collection || opts.Views || this.get('Views');
  var collection = {};

  if (options.isCollection) {
    collection = options;

  } else {
    opts.Item = opts.Item || this.get('Item');
    collection = new Collection(opts);
  }

  if (!options.isCollection) {
    this.extendViews(collection, opts);
  }

  this.emit('collection', collection, opts);
  return collection;
};

/**
 * Create a new view collection to be stored on the `app.views` object. See
 * the [create docs](docs/collections.md#create) for more details.
 *
 * @param  {String} `name` The name of the collection to create. Plural or singular form may be used, as the inflections are automatically resolved when the collection
 * is created.
 * @param  {Object} `opts` Collection options
 * @return {Object} Returns the `collection` instance for chaining.
 * @api public
 */

Templates.prototype.create = function(name, options) {
  debug('creating view collection: "%s"', name);
  options = options || {};

  // emit the collection name and options
  this.emit('create', name, options);

  // create the actual collection
  var collection = this.collection(options, true);
  utils.setInstanceNames(collection, name);

  // get the collection inflections, e.g. page/pages
  var single = utils.single(name);
  var plural = utils.plural(name);

  // map the inflections for lookups
  this.inflections[single] = plural;

  // add inflections to collection options
  collection.option('inflection', single);
  collection.option('plural', plural);

  // prime the viewType(s) for the collection
  this.viewType(plural, collection.viewType());

  // add a reference the collection's views on `app.views[plural]`
  this.views[plural] = collection.views;

  // create loader functions for adding views to the collection
  var parent = Object.create(collection);
  this.define(plural, function() {
    return parent.addViews.apply(parent, arguments);
  });
  this.define(single, function() {
    return parent.addView.apply(parent, arguments);
  });

  Object.setPrototypeOf(this[plural], parent);
  Object.setPrototypeOf(this[single], parent);

  // create references to the app methods on the collection
  // itself, so that chaining will work seamlessly
  collection.define(plural, this[plural]);
  collection.define(single, this[single]);

  // decorate collection and views in collection
  // (this is a prototype method to allow overriding behavior)
  this.extendViews(collection, options);

  // run collection plugins
  this.run(collection);

  // add collection and view helpers
  helpers.singular(this, collection);
  helpers.plural(this, collection);

  // emit create
  this.emit('postCreate', collection, options);
  return collection;
};

/**
 * Decorate or override methods on a view created by a collection.
 */

Templates.prototype.extendView = function(view, options) {
  plugin.view(this, view, options);
  return this;
};

/**
 * Decorate or override methods on a view collection instance.
 */

Templates.prototype.extendViews = function(views, options) {
  plugin.views(this, views, options);
  return this;
};

/**
 * Decorate or override methods on a view collection instance.
 */

Templates.prototype.extendList = function(views, options) {
  plugin.list(this, views, options);
  return this;
};

/**
 * Resolve the name of the layout to use for `view`
 */

Templates.prototype.resolveLayout = function(view, options) {
  debug('resolving layout for "%s"', view.key);
  var opts = Object.assign({}, options);

  if (!utils.isPartial(view) && typeof view.layout === 'undefined') {
    if (view.options && view.options.collection) {
      var collection = this[view.options.collection];
      var layout = collection.resolveLayout(view);
      if (typeof layout === 'undefined') {
        layout = opts.layout || this.option('layout');
      }
      return layout;
    }
  }
  return view.layout;
};

/**
 * Expose static `setup` method for providing access to an
 * instance before any other code is run.
 *
 * ```js
 * function App(options) {
 *   Templates.call(this, options);
 *   Templates.setup(this);
 * }
 * Templates.extend(App);
 * ```
 * @param {Object} `app` Application instance
 * @param {String} `name` Optionally pass the constructor name to use.
 * @return {undefined}
 * @api public
 */

Templates.setup = function(app, name) {
  var setup = app.options['init' + name || app.constructor.name];
  if (typeof setup === 'function') {
    setup.call(app, app, app.options);
  }
};

/**
 * Expose constructors as static methods.
 */

Templates.Base = Base;
Templates.Collection = lib.collection;
Templates.List = lib.list;
Templates.Group = lib.group;
Templates.Views = lib.views;
Templates.Item = lib.item;
Templates.View = lib.view;

/**
 * Expose properties for unit tests
 */

utils.define(Templates, 'utils', utils);
utils.define(Templates, '_', { lib: lib, plugin: plugin });
