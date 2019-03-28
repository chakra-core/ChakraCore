'use strict';

var utils = require('../utils');
var merge = require('../merge');

module.exports = function(val, key, config, schema) {
  merge(config, schema);
  val = val || config[key];

  if (utils.isString(val)) {
    return val;
  }

  if (schema.checked[key]) {
    return val;
  }

  schema.update('repository', config);
  var res = config.name || utils.repo.name(process.cwd());
  schema.checked[key] = true;
  return res;
};
