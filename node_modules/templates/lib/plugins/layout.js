'use strict';

var debug = require('debug')('base:templates:layout');
var utils = require('../utils');

module.exports = function(proto) {

  /**
   * Apply a layout to the given `view`.
   *
   * @name .applyLayout
   * @param  {Object} `view`
   * @return {Object} Returns a `view` object.
   */

  proto.applyLayout = function(view, locals) {
    debug('applying layout to view "%s"', view.path);
    if (view.options.layoutApplied) {
      return view;
    }

    // handle pre-layout middleware
    this.handleOnce('preLayout', view);
    var app = this;
    var opts = {};

    utils.extend(opts, this.options);
    utils.extend(opts, view.options);
    utils.extend(opts, locals);

    if (typeof view.context === 'function') {
      utils.extend(opts, view.context());
    }

    // get the name of the first layout
    var name = this.resolveLayout(view, opts);

    // if still no layout is defined, bail out
    if (utils.isFalsey(name)) {
      return view;
    }

    // Handle each layout before it's applied to a view
    function handleLayout(obj, stats, depth) {
      var layoutName = obj.layout.basename;
      debug('applied layout (#%d) "%s", to view "%s"', depth, layoutName, view.path);

      view.currentLayout = obj.layout;
      view.define('layoutStack', stats.history);
      app.handleOnce('onLayout', view);
      delete view.currentLayout;
    }

    try {
      // build the layout stack
      var stack = buildStack(this, name, view);

      // actually apply the layout
      var res = utils.layouts(view.content, name, stack, opts, handleLayout);
      view.enable('layoutApplied');
      view.option('layoutStack', res.history);
      view.contents = new Buffer(res.result);

      // handle post-layout middleware
      this.handleOnce('postLayout', view);
    } catch (err) {
      if (this.hasListeners('error')) {
        this.emit('error', err);
      } else {
        throw err;
      }
    }
    return view;
  };

  /**
   * Asynchronously apply a layout to the given `view`.
   *
   * @name .applyLayoutAsync
   * @param {Object} `view`
   * @param {Function} `callback`
   */

  proto.applyLayoutAsync = function(view, locals, cb) {
    debug('applying layout to view "%s"', view.path);
    var app = this;

    if (typeof locals === 'function') {
      cb = locals;
      locals = {};
    }

    if (view.options.layoutApplied) {
      cb(null, view);
      return;
    }

    // handle pre-layout middleware
    this.handleOnce('preLayout', view, function(err, file) {
      if (err) return cb(err);

      var opts = {};
      utils.extend(opts, app.options);
      utils.extend(opts, file.options);
      utils.extend(opts, locals);

      if (typeof file.context === 'function') {
        utils.extend(opts, file.context());
      }

      // get the name of the first layout
      var name = app.resolveLayout(file, opts);

      // if still no layout is defined, bail out
      if (utils.isFalsey(name)) {
        cb(null, file);
        return;
      }

      // Handle each layout before it's applied to a file
      function handleLayout(obj, stats, depth) {
        var layoutName = obj.layout.basename;
        debug('applied layout (#%d) "%s", to file "%s"', depth, layoutName, file.path);

        file.currentLayout = obj.layout;
        file.define('layoutStack', stats.history);
        app.handleOnce('onLayout', file);
        delete file.currentLayout;
      }

      try {

        // build the layout stack
        var stack = buildStack(app, name, file);

        // actually apply the layout
        var res = utils.layouts(file.content, name, stack, opts, handleLayout);
        file.enable('layoutApplied');
        file.option('layoutStack', res.history);
        file.contents = new Buffer(res.result);

      } catch (err) {
        if (app.hasListeners('error')) {
          app.emit('error', err);
        }
        cb(err);
        return;
      }

      // handle post-layout middleware
      app.handleOnce('postLayout', file, cb);
    });
  };

  /**
   * Get the layout stack by creating an object from all
   * collections with the "layout" `viewType`
   *
   * @param {Object} `app`
   * @param {String} `name` The starting layout name
   * @param {Object} `view`
   * @return {Object} Returns the layout stack.
   */

  function buildStack(app, name, view) {
    var layoutExists = false;
    var registered = 0;
    var layouts = {};

    // get all collections with `viewType` layout
    var collections = app.viewTypes.layout;
    var len = collections.length;
    var idx = -1;

    while (++idx < len) {
      var collection = app[collections[idx]];

      // detect if at least one of the collections has
      // our starting layout
      if (!layoutExists && collection.getView(name)) {
        layoutExists = true;
      }

      // add the collection views to the layouts object
      for (var key in collection.views) {
        layouts[key] = collection.views[key];
        registered++;
      }
    }

    if (registered === 0) {
      throw app.formatError('layouts', 'registered', name, view);
    }

    if (layoutExists === false) {
      throw app.formatError('layouts', 'notfound', name, view);
    }
    return layouts;
  }
};
