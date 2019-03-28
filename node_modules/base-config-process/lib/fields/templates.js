'use strict';

var views = require('./views');

/**
 * Load templates onto the given collections. Alias for [views](#views).
 *
 * @name plugins
 * @api public
 */

module.exports = function(app, base, options) {
  return views(app, base, options);
};
