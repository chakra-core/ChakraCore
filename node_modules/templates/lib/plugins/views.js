'use strict';

var utils = require('../utils');

/**
 * The `views` plugin is used to bubble up events:
 *
 *     view => collection => app
 *
 * and to pass down sensible options:
 *
 *     app => collection => view
 */

module.exports = function(app, views, options) {
  if (!utils.isValid(views, 'templates-plugins-views', ['views'])) {
    return;
  }

  /**
   * If `renameKey` is not defined on collection options,
   * use the `app.renameKey` settings.
   */

  if (typeof views.options.renameKey !== 'function') {
    views.option('renameKey', function(key, view) {
      return app.renameKey(key, view);
    });
  }

  /**
   * Overwrite the collection's `extendView` method with
   * `app.extendView()`
   */

  utils.define(views, 'extendView', function() {
    return app.extendView.apply(app, arguments);
  });

  /**
   * Bubble up custom collection events to app, allowing the user
   * to do: `app.on('page',...)`, `app.on('partial', ...)` etc.
   */

  var singularName = views.options.inflection;
  if (singularName && app.isApp) {
    views.on(singularName, function() {
      app.emit.bind(app, singularName).apply(app, arguments);
    });
  }

  /**
   * Listen for `view`
   *  - bubble event from `views` up to `app`
   *  - set default engine if not defined
   *  - set default layout if not defined
   */

  views.on('view', function(view, collectionName, collection) {
    views.setType(view);

    // bind the `addView` method to allow chaining
    utils.define(view, 'addView', views.addView.bind(views));

    // pass the engine defined on `collection.options` to `view.options`
    if (!view.engine) {
      view.engine = views.options.engine;
    }
    app.extendView(view, options);
  });

  /**
   * Listen for `load`
   */

  views.on('load', function(view, collectionName, collection) {
    app.emit.bind(app, 'view').apply(app, arguments);
    app.emit.bind(app, 'load').apply(app, arguments);

    if (app.options.onLoad !== false
        && views.options.onLoad !== false
        && view.options.onLoad !== false) {
      app.handleOnce('onLoad', view, view.next);
    }
  });
};
