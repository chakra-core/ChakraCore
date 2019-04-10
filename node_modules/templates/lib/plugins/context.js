'use strict';

var debug = require('debug');
var View = require('vinyl-view');
var utils = require('../utils');

/**
 * Plugin for adding data and context related methods to
 * an instance of `app` or `collection`.
 */

module.exports = function(app) {
  var opts = utils.extend({namespace: true}, app.options);

  /**
   * Custom debug methods
   */

  debug.context = debug('base:templates:context');
  debug.helper = function(key) {
    return debug('base:templates:helper:', key);
  };

  /**
   * Set, get and load data to be passed to templates as
   * context at render-time.
   *
   * ```js
   * app.data('a', 'b');
   * app.data({c: 'd'});
   * console.log(app.cache.data);
   * //=> {a: 'b', c: 'd'}
   * ```
   *
   * @name .data
   * @param {String|Object} `key` Pass a key-value pair or an object to set.
   * @param {any} `val` Any value when a key-value pair is passed. This can also be options if a glob pattern is passed as the first value.
   * @return {Object} Returns an instance of `Templates` for chaining.
   * @api public
   */

  app.use(utils.baseData(opts));

  /**
   * Register a default data loader
   */

  app.dataLoader('json', function(str) {
    return JSON.parse(str);
  });

  /**
   * Build the context for the given `view` and `locals`.
   *
   * @name .context
   * @param  {Object} `view` The view being rendered
   * @param  {Object} `locals`
   * @return {Object} The object to be passed to engines/views as context.
   * @api public
   */

  app.define('context', function(view, locals) {
    // backwards support for `mergeContext`
    var fn = this.options.context || this.options.mergeContext;
    if (typeof fn === 'function') {
      return fn.apply(this, arguments);
    }
    if (typeof view.context !== 'function') {
      view.context = View.context;
    }
    return view.context(this.cache.data, locals);
  });

  /**
   * Bind context to helpers.
   */

  app.define('bindHelpers', function(view, context, isAsync) {
    debug.context('binding helpers for %s <%s>', view.options.inflection, view.basename);
    var optsHelpers = utils.isObject(this.options.helpers) ? this.options.helpers : {};

    // merge cached _synchronous_ helpers with helpers defined on `app.options`
    var helpers = utils.extend({}, optsHelpers, this._.helpers.sync);

    // add helpers defined on the context
    if (utils.isObject(context.helpers)) {
      helpers = utils.extend({}, helpers, context.helpers);
    }

    // if any _async_ helpers are defined, merge those onto the context LAST
    if (isAsync) {
      helpers = utils.extend({}, helpers, this._.helpers.async);
    }

    // create the "helper context" to be exposed as `this` inside helper functions
    var thisArg = new Context(this, view, context, helpers);

    // bind the context to helpers.
    context.helpers = utils.bindAll(helpers, thisArg, {
      bindFn: function(thisArg, key, parent, options) {
        thisArg.debug = debug.helper(key);
        thisArg.helper.name = key;
        setHelperOptions(thisArg, key);
        thisArg._parent = parent;
        thisArg = utils.extend({}, thisArg, parent);
        return thisArg;
      }
    });
  });

  /**
   * Update context in a helper so that `this.helper.options` is
   * the options for that specific helper.
   *
   * @param {Object} `context`
   * @param {String} `key`
   * @api public
   */

  function setHelperOptions(context, key) {
    var optsHelper = context.options.helper || {};
    if (optsHelper.hasOwnProperty(key)) {
      context.helper.options = optsHelper[key];
    }
  }

  /**
   * Create a new context object to expose inside helpers.
   *
   * ```js
   * app.helper('lowercase', function(str) {
   *   // the 'this' object is the _helper_ context
   *   console.log(this);
   *   // 'this.app' => the application instance, e.g. templates, assemble, verb etc.
   *   // 'this.view' => the current view being rendered
   *   // 'this.helper' => helper name and options
   *   // 'this.context' => view context (as opposed to _helper_ context)
   *   // 'this.options' => options created for the specified helper being called
   * });
   * ```
   * @param {Object} `app` The application instance
   * @param {Object} `view` The view being rendered
   * @param {Object} `context` The view's context
   * @param {Object} `options`
   */

  function Context(app, view, context, helpers) {
    this.helper = {};
    this.helper.options = createHelperOptions(app, view, helpers);

    this.context = context;
    utils.define(this.context, 'view', view);
    this.options = utils.merge({}, app.options, view.options, this.helper.options);

    // make `this.options.handled` non-enumberable
    utils.define(this.options, 'handled', this.options.handled);

    decorate(this.context);
    decorate(this.options);
    decorate(this);

    this.view = view;
    this.app = app;
    this.ctx = function() {
      return helperContext.apply(this, arguments);
    };
  }

  /**
   * Expose `this.ctx()` in helpers for creating a custom context object
   * from a `view` being rendered, `locals` and helper `options` (which
   * can optionally have a handlebars `options.hash` property)
   *
   * The context object is created from:
   *
   * - `this.context`: the context for the **current view being rendered**
   * - `this.view.locals`: the `locals` object for current view being rendered
   * - `this.view.data`: front-matter from current view being rendered
   * - `view.locals`: locals defined on the view being injected
   * - `view.data`: front-matter on the view being injected
   * - helper `locals`: locals passed to the helper
   * - helper `options`: options passed to the helper
   * - helper `options.hash` if helper is registered as a handlebars helper
   *
   * Also note that `view` is the view being injected, whereas `this.view`
   * is the `view` being rendered.
   *
   * ```js
   * // handlebars helper
   * app.helper('foo', function(name, locals, options) {
   *   var view = this.app.find(name);
   *   var ctx = this.ctx(view, locals, options);
   *   return options.fn(ctx);
   * });
   *
   * // async helper
   * app.helper('foo', function(name, locals, options, cb) {
   *   var view = this.app.find(name);
   *   var ctx = this.ctx(view, locals, options);
   *   view.render(ctx, function(err, res) {
   *     if (err) return cb(err);
   *     cb(null, res.content);
   *   });
   * });
   * ```
   * @name .this.ctx
   * @param {Object} `view` the view being injected
   * @param {Object} `locals` Helper locals
   * @param {Object} `options` Helper options
   * @return {Object}
   * @public
   */

  function helperContext(view, locals, options) {
    var fn = this.options.helperContext;
    var context = {};

    if (!utils.isObject(view) || !view.isView) {
      locals = view;
      options = locals;
      view = this.view;
    }

    options = options || {};
    locals = locals || {};
    if (typeof fn === 'function') {
      return fn.call(this, view, locals, options);
    }

    // merge "view" front-matter with context
    context = utils.merge({}, context, this.view.locals, this.view.data);

    // merge in partial locals and front-matter
    if (view !== this.view) {
      context = utils.merge({}, context, view.locals, view.data);
    }

    // merge in helper locals and options.hash
    context = utils.merge({}, context, locals, options.hash);
    return context;
  }

  /**
   * Decorate the given object with `merge`, `set` and `get` methods
   */

  function decorate(obj) {
    utils.define(obj, 'merge', function() {
      var args = [].concat.apply([], [].slice.call(arguments));
      var len = args.length;
      var idx = -1;

      while (++idx < len) {
        var val = args[idx];
        if (!utils.isObject(val)) continue;
        if (val.hasOwnProperty('hash')) {
          // shallow clone and delete the `data` object
          val = utils.merge({}, val, val.hash);
          delete val.data;
        }
        utils.merge(obj, val);
      }
      // ensure methods aren't overwritten
      decorate(obj);
      if (obj.hasOwnProperty('app') && obj.hasOwnProperty('options')) {
        decorate(obj.options);
      }
      return obj;
    });

    utils.define(obj, 'get', function(prop) {
      return utils.get(obj, prop);
    });

    utils.define(obj, 'set', function(key, val) {
      return utils.set(obj, key, val);
    });
  }

  /**
   * Support helper options defined on `app.options.helper`. For example,
   * to define options for helper `foo`:
   *
   * ```js
   * app.option('helper.foo', {doStuff: true});
   * ```
   * @param {Object} `app`
   * @param {Object} `helpers` Currently defined helpers, to match up options
   * @return {Object} Returns helper options
   */

  function createHelperOptions(app, view, helpers) {
    var options = utils.merge({}, app.options, view.options);
    var helperOptions = {};

    if (options.hasOwnProperty('helper')) {
      var opts = options.helper;
      if (!utils.isObject(opts)) {
        return helperOptions;
      }

      for (var key in opts) {
        if (opts.hasOwnProperty(key) && helpers.hasOwnProperty(key)) {
          helperOptions[key] = opts[key];
        }
      }
    }
    return helperOptions;
  }

  /**
   * Merge "partials" view types. This is necessary for template
   * engines have no support for partials or only support one
   * type of partials.
   *
   * @name .mergePartials
   * @param {Object} `options` Optionally pass an array of `viewTypes` to include on `options.viewTypes`
   * @return {Object} Merged partials
   * @api public
   */

  function mergePartials(options) {
    var opts = utils.merge({}, this.options, options);
    var names = opts.mergeTypes || this.viewTypes.partial;
    var partials = {};
    var self = this;

    names.forEach(function(name) {
      var collection = self.views[name];
      for (var key in collection) {
        var view = collection[key];

        // handle `onMerge` middleware
        self.handleOnce('onMerge', view, function(err, res) {
          if (err) throw err;
          view = res;
        });

        if (view.options.nomerge) continue;
        if (opts.mergePartials !== false) {
          name = 'partials';
        }

        // convert the partial to:
        //=> {'foo.hbs': 'some content...'};
        partials[name] = partials[name] || {};
        partials[name][key] = view.content;
      }
    });
    return partials;
  };

  /**
   * Merge "partials" view types. This is necessary for template engines
   * have no support for partials or only support one type of partials.
   *
   * @name .mergePartialsAsync
   * @param {Object} `options` Optionally pass an array of `viewTypes` to include on `options.viewTypes`
   * @param {Function} `callback` Function that exposes `err` and `partials` parameters
   * @api public
   */

  mergePartials.async = function(options, done) {
    if (typeof options === 'function') {
      done = options;
      options = {};
    }

    var opts = utils.merge({}, this.options, options);
    var names = opts.mergeTypes || this.viewTypes.partial;
    var partials = {};
    var self = this;

    utils.each(names, function(name, cb) {
      var collection = self.views[name];
      var keys = Object.keys(collection);

      utils.each(keys, function(key, next) {
        var view = collection[key];

        // handle `onMerge` middleware
        self.handleOnce('onMerge', view, function(err, file) {
          if (err) return next(err);

          if (file.options.nomerge) {
            return next();
          }

          if (opts.mergePartials !== false) {
            name = 'partials';
          }

          // convert the partial to:
          //=> {'foo.hbs': 'some content...'};
          partials[name] = partials[name] || {};
          partials[name][key] = file.content;
          next();
        });
      }, cb);
    }, function(err) {
      if (err) return done(err);
      done(null, partials);
    });
  };

  /**
   * Expose `mergePartials` functions as methods
   */

  app.define('mergePartials', mergePartials);
  app.define('mergePartialsAsync', mergePartials.async);
};
