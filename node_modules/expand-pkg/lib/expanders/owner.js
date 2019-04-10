'use strict';

var utils = require('../utils');

module.exports = function(val, key, config, schema) {
  if (utils.isString(val)) {
    config[key] = val;
    return val;
  }

  // ensure the necessary properties are defined to determine owner
  schema.update('name', config);
  schema.update('repository', config);
  schema.update('git', config);

  return config.owner
    || config.username
    || config.author && config.author.username;
};
