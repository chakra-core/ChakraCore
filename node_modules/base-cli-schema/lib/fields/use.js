'use strict';

var path = require('path');
var util = require('util');
var utils = require('../utils');

module.exports = function(app, base, options) {
  return function(val, key, config, schema) {
    if (typeof val === 'undefined') {
      return;
    }

    if (typeof val === 'string') {
      val = val.split(',');
    }

    if (!Array.isArray(val)) {
      val = util.inspect(val);
      throw new TypeError('--' + key + ': expected a string or boolean, but received: ' + val);
    }

    if (config.cwd) {
      app.cwd = path.resolve(app.cwd, path.resolve(config.cwd));
    }

    config[key] = val.map(function(name) {
      return utils.tryResolve(name, app.cwd);
    });
    return config[key];
  };
};
