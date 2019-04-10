'use strict';

var debug = require('debug')('base:cli');
var utils = require('./utils');

module.exports = function(app) {
  return function(val, key, config, schema) {
    debug('command > %s: "%j"', key, val);
    if (typeof val === 'undefined') {
      schema.omit(key);
      return;
    }

    if (utils.typeOf(val) === 'object') {
      return utils.stringify(val);
    }

    if (typeof val === 'string') {
      return val;
    }

    schema.omit(key);
    return;
  };
};

