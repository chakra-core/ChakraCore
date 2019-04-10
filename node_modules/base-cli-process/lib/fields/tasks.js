'use strict';

var debug = require('../debug');

module.exports = function(app) {
  return function(val, key, config, next) {
    debug.field(key, val);

    if (config.run === false) {
      config[key] = [];
    }

    next();
  };
};
