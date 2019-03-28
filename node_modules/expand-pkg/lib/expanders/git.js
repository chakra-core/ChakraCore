'use strict';

var utils = require('../utils');

module.exports = function(val, key, config, schema) {
  if (!utils.isObject(val)) {
    var git = utils.parseGitConfig.sync(val);
    var obj = utils.parseGitConfig.keys(git);
    git = utils.merge({}, git, obj);
    val = config[key] = git;
  }

  schema.update('repository', config);

  if (!utils.isString(config.repository)) {
    config.repository = utils.get(config, 'git.remote.origin.url');
  }

  if (utils.isString(config.repository)) {
    utils.defaults(config, utils.repo.expandUrl(config.repository, config));
  }

  schema.update('homepage', config);
  return val;
};
