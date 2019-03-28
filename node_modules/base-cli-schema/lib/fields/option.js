'use strict';

var utils = require('../utils');

module.exports = function(app, base, argv) {
  return function(val, key, config, schema) {
    if (typeof val === 'undefined') {
      return;
    }

    if (typeof val === 'boolean') {
      val = { show: val };
    }

    if (typeof val === 'string') {
      var obj = {};
      obj[val] = true;
      val = config[key] = obj;
    }

    if (!utils.isObject(val)) {
      delete config[key];
      return;
    }

    schema.update('options', config);
    config.options = utils.merge({}, config.options, val);
    schema.omit(key);
    return;
  };
};
