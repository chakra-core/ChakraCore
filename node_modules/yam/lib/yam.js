'use strict';

function Yam(id, options) {
  if (!id) { throw new Error('You must specify an id.'); }

  var path = require('path');
  var merge = require('lodash.merge');
  var Config = require('./config');
  var configUtils = require('./utils/config-utils');
  var projectPath  = configUtils.getCurrentPath();
  var homePath     = configUtils.getUserHomeDirectory();
  var localOptions = {};

  var localHomePath   = homePath + '/.' + id;
  var localProjectPath = projectPath + '/.' + id;

  if (options && options.primary) {
    localProjectPath = options.primary + '/.' + id;
  }

  if (options && options.secondary) {
    localHomePath = options.secondary + '/.' + id;
  }

  localOptions = {
    primary:   path.normalize(localProjectPath),
    secondary: path.normalize(localHomePath)
  };

  this.id = id;

  this.options = merge(
    new Config(localOptions.secondary),
    new Config(localOptions.primary)
  );

  return this;
}

Yam.prototype.getAll = function getAll() {
  return this.options;
};

Yam.prototype.get = function get(key) {
  var value = this.options[key];

  return value !== undefined ? value : undefined;
};

module.exports = Yam;
