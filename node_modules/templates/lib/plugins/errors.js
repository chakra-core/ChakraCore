'use strict';

var util = require('util');
var utils = require('../utils');

module.exports = function(proto, name) {
  if (!name && proto.constructor) {
    name = proto.constructor.name;
  }

  /**
   * Format an error
   */

  proto.formatError = function(method, id, msg, view) {
    var ctx = this.templatesError[method][id];
    if (!view) view = {relative: ''};
    if (!msg) msg = '';

    var str = name + '#' + method + ' ' + ctx;
    var reason = util.format(str, msg, view.relative).trim();

    var err = new Error(reason);
    err.reason = reason;
    err.id = id;
    err.msg = msg;
    return err;
  };

  /**
   * Rethrow an error in the given context to
   * get better error messages.
   */

  proto.rethrow = function(method, err, view, context) {
    if (this.options.rethrow !== true) return err;

    try {
      var opts = utils.extend({}, this.options.rethrow, {
        data: context,
        fp: view.path
      });

      utils.rethrow(view.content, opts);
    } catch (e) {
      err.method = method;
      err.reason = e;
      err.view = view;
      err.id = 'rethrow';
      err._called = true;
      return err;
    }
  };
};
