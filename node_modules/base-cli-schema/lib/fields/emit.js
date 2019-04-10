'use strict';

var utils = require('../utils');

module.exports = function(app, base, options) {
  return function(val, key, config, next) {
    if (typeof val === 'undefined') {
      return;
    }

    if (utils.isObject(val)) {
      val = utils.stringify(val);
    }

    val = utils.arrayify(val);
    if (val.length === 0) {
      val = ['*'];
    }
    return val;
  };
};
