'use strict';

var utils = require('../utils');

module.exports = function(app) {
  return {
    type: ['array', 'string'],
    normalize: function(val, key, config, schema) {
      return utils.arrayify(val);
    }
  };
};
