'use strict';

var debug = require('./debug');
var utils = require('./utils');

module.exports = function(app, prop) {
  return function(val, key, config, next) {
    debug.field(key, val);

    if (utils.isString(val)) {
      val = config[key] = [utils.stripQuotes(val)];
    }

    var obj = {};
    obj[prop] = val;
    app.config.process(obj, next);
  }
};
