'use strict';

var utils = require('../utils');
var merge = require('../merge');

module.exports = function(val, key, config, schema) {
  merge(config, schema);
  val = val || config[key];

  switch (utils.typeOf(val)) {
    case 'object':
      return val.type;
    case 'array':
      return val[0].type;
    case 'string':
    default: {
      return val;
    }
  }
};
