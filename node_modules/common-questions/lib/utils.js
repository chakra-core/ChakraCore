'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('base-pkg', 'pkg');
require('base-project', 'project');
require('base-questions', 'questions');
require('camel-case', 'camelcase');
require('common-config', 'common');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('is-valid-app', 'isValid');
require('mixin-deep', 'merge');
require('omit-empty');
require('set-value', 'set');
require('get-value', 'get');
require = fn;

utils.getProjectName = function(app, data) {
  return utils.get(data, 'name') || app.pkg.get('name') || app.project;
};

utils.getProjectAlias = function(app, data, name) {
  name = name || utils.getProjectName(app, data);
  if (typeof name === 'undefined') return;
  if (typeof app.toAlias === 'function') {
    return app.toAlias.call(app, name);
  }
  if (typeof app.options.toAlias === 'function') {
    return app.options.toAlias.call(app, name);
  }
  if (app.aliasRegex instanceof RegExp) {
    return utils.camelcase(name.replace(app.aliasRegex, ''));
  }
  return utils.camelcase(name);
};

utils.getProjectOwner = function(app, data) {
  var name = utils.getProjectName(app, data);
  if (/^base-/.test(name)) {
    return 'node-base';
  }
  if (/^assemble-/.test(name)) {
    return 'assemble';
  }
  if (/^generate-/.test(name)) {
    return 'generate';
  }
  if (/^(helper|handlebars-helper)-/.test(name)) {
    return 'helpers';
  }
  if (/^updater-/.test(name)) {
    return 'update';
  }
  if (/^verb-/.test(name)) {
    return 'verbose';
  }

  var owner = app.get('cache.data.owner');
  if (owner) {
    return owner;
  }

  var repo = utils.get(data, 'repository') || app.pkg.get('repository');
  if (typeof repo === 'string' && !/:/.test(repo)) {
    var segs = repo.split('/');
    if (segs.length === 2) {
      return segs.shift();
    }
  }
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
