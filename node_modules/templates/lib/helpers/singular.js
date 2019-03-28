'use strict';

var debug = require('debug')('base:templates:helper');
var utils = require('../utils');

module.exports = function(app, collection) {
  var single = collection.options.inflection;

  /**
   * Create an async helper for getting a view from a collection.
   *
   * ```html
   * <%%= page("foo.tmpl") %>
   * ```
   * @param {String} `name` The name or path of the view to get.
   * @param {String} `singular` Singular name of the collection.
   * @api public
   */

  app.asyncHelper(single, function viewHelper(name, locals, options, cb) {
    this.app.emit('helper', single + ' helper > rendering "' + name + '"');
    debug('"%s" searching for "%s"', single, name);

    if (typeof locals === 'function') {
      return viewHelper.call(this, name, {}, {}, locals);
    }

    if (typeof options === 'function') {
      if (typeof locals === 'object' && typeof locals.hash === 'object') {
        return viewHelper.call(this, name, {}, locals, options);
      }
      return viewHelper.call(this, name, locals, {}, options);
    }

    options = options || {};
    locals = locals || {};

    var opts = helperOptions.call(this, locals, options);
    var views = this.app[collection.options.plural] || app[collection.options.plural];

    var view = views.getView(name, opts);
    if (!view) {
      utils.helperError(this.app, single, name, cb);
      return;
    }

    debug('"%s" pre-rendering "%s"', single, name);

    var context = this.ctx(view, locals, opts);
    view._context = utils.merge({}, this.context, context);

    view.render(function(err, res) {
      if (err) return cb(err);

      debug('"%s" rendered "%s"', single, name);
      cb(null, res.content);
    });
  });
};

/**
 * Create an options object from:
 *
 * - helper `locals`
 * - helper `options`
 * - options `hash` if it's registered as a handlebars helper
 * - context options (`this.options`), created from options define on `app.option()`.
 *
 * @param {Object} `locals`
 * @param {Object} `options`
 * @return {Object}
 */

function helperOptions(locals, options) {
  var hash = options.hash || locals.hash || {};
  var opts = utils.merge({}, this.options, hash);
  opts.hash = hash;
  return opts;
}
