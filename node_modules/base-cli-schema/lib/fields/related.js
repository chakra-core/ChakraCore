'use strict';

var utils = require('../utils');

module.exports = function(app, options) {
  return function(val, key, config, schema) {
    if (typeof val === 'undefined') {
      return;
    }

    if (val === true) {
      return { show: true };
    }

    if (typeof val === 'string') {
      val = val.split(',');
    }

    if (Array.isArray(val)) {
      val = { list: val };
    }

    if (utils.isObject(val) && val.list) {
      if (typeof val.list === 'string') {
        val.list = val.list.split(',');
      }

      // we might have something like `{related: {list: {foo: true}}}`
      if (utils.isObject(val.list)) {
        val.list = Object.keys(val.list);
      }

      val.list = utils.unique(utils.unify(val.list));
      val.list.sort();
    }

    config[key] = val;
    return val;
  };
};
