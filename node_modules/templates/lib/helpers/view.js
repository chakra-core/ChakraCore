'use strict';

var debug = require('debug')('base:templates:helper');
var utils = require('../utils');

module.exports = function(app) {

  /**
   * Get a specific view by `name`, optionally specifying
   * the view's collection as the second argument, to limit
   * (and potentially speed up) the lookup.
   *
   * ```html
   * <%%= view("foo") %>
   * ```
   * @param {String} `name`
   * @param {String} `collection`
   * @return {String}
   * @api public
   */

  app.asyncHelper('view', function(name, collectionName, locals, cb) {
    debug('view helper, getting "%s"', name);
    var args = [].slice.call(arguments, 1);

    var last = utils.last(args);
    if (typeof last === 'function') {
      cb = args.pop();
    }

    last = utils.last(args);
    if (utils.isObject(last)) {
      locals = args.pop();
    } else {
      collectionName = args.pop();
    }

    var view = this.app.find(name, collectionName) || app.find(name, collectionName);

    // if no view is found, return an empty string
    if (!view) {
      cb(null, '');
      return;
    }

    // create the context
    var ctx = this.context.merge(locals);

    // render the view
    view.render(ctx, function(err, res) {
      if (err) return cb(err);

      debug('"view" rendered helper "%s"', res.relative);
      cb(null, res.content);
    });
  });
};
