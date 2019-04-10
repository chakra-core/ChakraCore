'use strict';

var debug = require('debug')('base:templates:helper');
var List = require('../list');

module.exports = function(app, collection) {
  var plural = collection.options.plural;

  /**
   * Creates an async helper for each view collection, and exposes
   * the collection as an instance of `List` to the helper context.
   *
   * If the helper is used as a handlebars block helper, the collection
   * array will be exposed as context to `options.fn()`, otherwise
   * the `List` array is returned.
   *
   * ```js
   * // create a collection
   * app.create('pages');
   *
   * // use the `pages` helper
   * // {{#pages}} ... {{/pages}}
   * ```
   * @param {Object} `options` Locals or handlebars options object
   * @param {String} `callback` Handled internally by the templates library.
   * @api public
   */

  app.helper(plural, function listHelper(options) {
    debug('list helper "%s", "%j"', plural, options);
    var list = new List(collection);

    // expose `items` as the plural name of the collection
    if (plural !== 'items') {
      list[plural] = list.items;
    }

    // render block helper with list as context
    if (options && typeof options.fn === 'function' && options.hash) {
      return options.fn(list)
    }

    // return list when not used as a block helper
    return list;
  });
};
