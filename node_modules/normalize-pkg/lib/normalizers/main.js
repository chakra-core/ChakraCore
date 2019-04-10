'use strict';

var utils = require('../utils');
var merge = require('../merge');

module.exports = function(val, key, config, schema) {
  merge(config, schema);
  val = val || config[key];

  if (typeof val === 'undefined') {
    val = 'index.js';
  }

  if (!utils.exists(val)) {
    delete config[key];
    return;
  }

  schema.update('files', config);
  schema.union('files', config, val);
  return val;
};
