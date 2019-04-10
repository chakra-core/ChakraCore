'use strict';

var utils = require('../utils');

module.exports = function(app, options) {
  return function(val, key, config, schema) {
    if (typeof val === 'undefined') {
      delete config[key];
      return;
    }

    if (utils.isObject(val)) {
      var tasks = utils.tableize(val);
      var arr = [];

      for (var prop in tasks) {
        if (tasks.hasOwnProperty(prop)) {
          arr.push(prop + ':' + tasks[prop]);
        }
      }

      val = config[key] = arr;
    }

    if (typeof val === 'boolean') {
      config.run = val;
      delete config[key];
      return;
    }

    if (typeof val === 'string') {
      val = val.split(',');
    }

    val = utils.unify(val);
    config[key] = val;
    return val;
  };
};
