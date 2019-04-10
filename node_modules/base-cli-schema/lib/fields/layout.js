'use strict';

var utils = require('../utils');

module.exports = function(app) {
  return function(val, key, config, schema) {
    if (typeof val === 'undefined') {
      return;
    }

    if (utils.falsey(val) || val === 'null') {
      return false;
    }

    if (val === true) {
      return 'default';
    }

    if (utils.isObject(val)) {
      val = config[key] = utils.stringify(val);
    }
    return val;
  };
};
