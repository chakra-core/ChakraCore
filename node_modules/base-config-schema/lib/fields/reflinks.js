'use strict';

var utils = require('../utils');

module.exports = function(app, options) {
  return function(val, key, config, schema) {
    if (!val || utils.isEmpty(val)) return null;
    if (typeof val === 'string') {
      val = val.split(',').filter(Boolean);
    }
    return utils.unique(utils.flatten(utils.arrayify(val)));
  };
};
