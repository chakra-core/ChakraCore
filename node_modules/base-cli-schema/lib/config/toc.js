'use strict';

var utils = require('../utils');

module.exports = function(app) {
  return {
    type: ['object', 'string'],
    normalize: function(val, key, config, schema) {
      if (typeof val === 'undefined') {
        return;
      }
      if (val === 'render') {
        val = config[key] = true;
      }
      if (typeof val === 'boolean') {
        return val;
      }
      if (utils.isObject(val) && val.hasOwnProperty('render')) {
        val = config[key] = val.render;
        return val;
      }
      delete config[key];
    }
  };
};
