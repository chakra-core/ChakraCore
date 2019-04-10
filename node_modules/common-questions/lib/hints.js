'use strict';

var utils = require('./utils');
var hints = module.exports;

hints.license = function(app) {
  return get(app, 'license') || 'MIT';
};

hints.name = function(app) {
  return utils.getProjectName(app, getData(app));
};

hints.alias = function(app) {
  var data = getData(app);
  return utils.getProjectAlias(app, data, data.name);
};

hints.owner = function(app) {
  return utils.getProjectOwner(app, getData(app));
};

hints.description = function(app) {
  return get(app, 'description') || app.pkg.get('description');
};

hints.homepage = function(app) {
  var owner = get(app, 'owner') || get(app, 'author.username');
  var name = get(app, 'name');
  if (owner && name) {
    return `https://github.com/${owner}/${name}`;
  }
};

hints.version = function(app) {
  return app.pkg.get('version') || get(app, 'version') || '0.1.0';
};

hints['author.username'] = function(app) {
  return get(app, 'author.username') || get(app, 'owner');
};
hints['author.twitter'] = function(app) {
  return get(app, 'author.twitter') || get(app, 'username');
};
hints['author.email'] = function(app) {
  return get(app, 'author.email');
};
hints['author.name'] = function(app) {
  return get(app, 'author.name');
};
hints['author.url'] = function(app) {
  return get(app, 'author.url');
};

function getData(app) {
  return utils.merge({}, app.base.questions.hints.data, app.base.cache.data, app.base.cache.answers);
}

function get(app, key) {
  return utils.get(getData(app), key);
}
