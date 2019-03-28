'use strict';

var utils = require('../utils');

/**
 * This plugin is used by collections and the main application
 * instance to ensure that certain methods and settings will
 * always exist on views.
 */

module.exports = function decorateView(app, view, options) {
  if (!utils.isValid(view, 'templates-plugins-view', ['view', 'item'])) {
    return;
  }

  utils.define(view, 'compile', function() {
    app.compile.bind(app, this).apply(app, arguments);
    return this;
  });

  utils.define(view, 'render', function() {
    app.render.bind(app, this).apply(app, arguments);
    return this;
  });
};
