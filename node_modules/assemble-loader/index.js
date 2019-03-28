'use strict';

var path = require('path');
var utils = require('./utils');

function loader(patterns, config) {
  if (utils.isObject(patterns)) {
    config = patterns;
    patterns = undefined;
  }

  return function plugin(app) {
    if (utils.isRegistered(this, 'assemble-loader')) return;

    // register the plugin on an "app"
    if (utils.isValidInstance(this)) {
      appLoader(this, config);

      // if patterns are passed to the plugin, load them now
      if (utils.isValidGlob(patterns)) {
        this.load(patterns);
      }
    }

    // register the plugin on a "collection"
    if (utils.isValidInstance(this, ['collection'])) {
      collectionLoader(this, config);

      // if patterns are passed to the plugin, load them now
      if (utils.isValidGlob(patterns)) {
        this.load(patterns);
      }
    }

    return plugin;
  };
}

/**
 * Adds a `.load` method to the "app" instance for loading views that
 * that don't belong to any particular collection. It just returns the
 * object of views instead of caching them.
 *
 * ```js
 * var loader = require('assemble-loader');
 * var assemble = require('assemble');
 * var app = assemble();
 * app.use(loader());
 *
 * var views = app.load('foo/*.hbs');
 * console.log(views);
 * ```
 * @param {Object} `app` application instance (e.g. assemble, verb, etc)
 * @param {Object} `config` Settings to use when registering the plugin
 * @return {Object} Returns an object of _un-cached_ views, from a glob, string, array of strings, or objects.
 * @api public
 */

function appLoader(app, config) {
  app.define('load', load('view', config));
  var fn = app.view;

  app.define('view', function() {
    var view = fn.apply(this, arguments);
    utils.contents.sync(view);
    return view;
  });
}

/**
 * Collection loaders
 */

function collectionLoader(collection, config) {
  collection._addView = collection.addView.bind(collection);
  var fn = collection.view;

  collection.define('view', function() {
    var view = fn.apply(this, arguments);
    utils.contents.sync(view);
    return view;
  });

  /**
   * Patches the `.addViews` method to support glob patterns.
   *
   * @param {Object|String} `key` View name or object.
   * @param {Object} `value` View options, when key is a string.
   * @return {Object}
   */

  collection.define('addView', function(key, value) {
    return this._addView.apply(this, arguments);
  });

  /**
   * Patches the `.addViews` method to support glob patterns.
   *
   * @param {Object|String} `key` View name or object.
   * @param {Object} `value` View options, when key is a string.
   * @return {Object}
   */

  collection.define('addViews', function(key, value) {
    this.load.apply(this, arguments);
    return this;
  });

  collection.define('loadView', function(filepath, options) {
    this.load.apply(this, arguments);
    return this;
  });

  collection.define('loadViews', function(patterns, options) {
    this.load.apply(this, arguments);
    return this;
  });

  collection.define('load', function() {
    return load('_addView', config).apply(this, arguments);
  });
}

/**
 * Create options from:
 *   + `config` - settings passed when registering plugin
 *   + `app.options` - options set on the instance
 *   + `options` - options passed when calling a loader method
 */

function mergeOptions(app, config, options) {
  if (utils.isView(options)) options = {};
  var opts = utils.extend({}, config, app.options, options);
  opts.cwd = path.resolve(opts.cwd || app.cwd || process.cwd());
  return opts;
}

/**
 * Create a `Loader` instance with a `loaderfn` bound
 * to the app or collection instance.
 */

function createLoader(options, fn) {
  var loader = new utils.Loader(options);
  return function() {
    if (!this.isApp) loader.cache = this.views;
    loader.options.loaderFn = fn.bind(this);
    loader.load.apply(loader, arguments);
    return loader.cache;
  };
}

/**
 * Create a function for loading views using the given
 * `method` on the collection or app.
 */

function load(method, config) {
  return function(patterns, options) {
    var opts = mergeOptions(this, config, options);
    var loader = createLoader(opts, this[method]);
    return loader.apply(this, arguments);
  };
}

/**
 * Expose `loader`
 */

module.exports = loader;
