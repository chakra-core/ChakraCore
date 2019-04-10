'use strict';

var git = require('./helpers/git');
var getOwner = require('./helpers/owner');
var utils = require('../utils');
var merge = require('../merge');

module.exports = function(val, key, config, schema) {
  merge(config, schema);

  val = val || config[key];
  if (schema.checked[key]) {
    return val;
  }

  if (utils.isObject(val) && val.url) {
    val = val.url;
  }

  if (!utils.isString(val)) {
    var parsed = git(val, key, config, schema);
    var remote = utils.get(parsed, 'remote.origin.url');
    if (remote) {
      var expanded = utils.repo.expandUrl(remote);
      var data = utils.merge({}, expanded, config);
      utils.merge(config, data);
      schema.data.merge(data);
    }
  }

  if (!utils.isString(val) && config.homepage) {
    val = config.homepage;
  }

  var owner = config.owner || getOwner(val, key, config, schema);
  if (config.name && owner) {
    val = owner + '/' + config.name;
  }

  if (utils.isString(val)) {
    schema.checked[key] = true;
    return utils.repo.repository(val);
  }

  // if not returned already, val is invalid
  delete config[key];
  return;
};
