'use strict';

var utils = require('../utils');

module.exports = function(val, key, config, schema) {
  if (utils.isString(val)) {
    config[key] = val;
    return val;
  }

  schema.update('repository', config);

  if (!utils.isString(config.repository) && config.owner) {
    config.repository = utils.repo.repository(config);
  }

  if (utils.isString(config.repository)) {
    return utils.repo.homepage(config.repository, config);
  }
};
