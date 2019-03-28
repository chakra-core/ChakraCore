'use strict';

var utils = require('../utils');

module.exports = function(app, options) {
  return function(val, key, config, schema) {
    if (!val || utils.isEmpty(val)) return null;
    if (typeof val === 'string') {
      val = val.split(',').filter(Boolean);
    }

    if (Array.isArray(val)) {
      val = { list: val };
    }

    if (utils.isObject(val) && typeof val.list === 'string') {
      val.list = val.list.split(',').filter(Boolean);
    }

    if (!val.list) {
      val.list = [];
      return val;
    }

    val.list = utils.unique(utils.flatten(val.list));
    return val;
  };
};
