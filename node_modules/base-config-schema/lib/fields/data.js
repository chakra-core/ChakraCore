'use strict';

var debug = require('../debug');
var utils = require('../utils');

module.exports = function(app) {
  return function(val, key, config, schema) {
    if (!val || utils.isEmpty(val)) return null;
    debug.field(key, val);

    if (val === true) {
      val = { show: true };
    }

    if (!utils.isObject(val)) {
      delete config[key];
      return;
    }
    return val;
  };
};
