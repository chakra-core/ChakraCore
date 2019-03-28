'use strict';

var utils = require('../utils');

module.exports = function(app, options) {
  return function(val, key, config, schema) {
    if (!val || utils.isEmpty(val)) return null;

    if (typeof val === 'string') {
      return val.split(',');
    }

    return utils.unique(utils.flatten(val));
  };
};
