'use strict';

var utils = require('../utils');

/**
 * The `list` plugin is used to bubble up events:
 *
 *     item => list => app
 *
 * and to pass down sensible options:
 *
 *     app => list => item
 */

module.exports = function(app, list, options) {

  /**
   * If `renameKey` is not defined on list options,
   * use the `app.renameKey` settings.
   */

  if (typeof list.options.renameKey !== 'function') {
    list.option('renameKey', function(key, item) {
      return app.renameKey(key, item);
    });
  }

  /**
   * Overwrite the list's `extendItem` method with
   * `app.extendItem()`
   */

  utils.define(list, 'extendItem', function() {
    return app.extendView.apply(app, arguments);
  });

  /**
   * Listen for `item`
   *  - bubble event from `list` up to `app`
   *  - set default engine if not defined
   *  - set default layout if not defined
   */

  list.on('item', function(item, list) {
    // bind the `addItem` method to allow chaining
    utils.define(item, 'addItem', list.addItem.bind(list));

    // pass the engine defined on `list.options` to `item.options`
    if (!item.engine) item.engine = list.options.engine;
    app.extendView(item, options);
  });

  /**
   * Listen for `load`
   */

  list.on('load', function(item, list) {
    app.emit.bind(app, 'item').apply(app, arguments);
    app.emit.bind(app, 'load').apply(app, arguments);

    if (app.options.onLoad !== false && list.options.onLoad !== false && item.options.onLoad !== false) {
      app.handleOnce('onLoad', item);
    }
  });
};
