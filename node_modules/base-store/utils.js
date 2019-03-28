'use strict';

var debug = require('debug')('base:store');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('data-store', 'store');
require('extend-shallow', 'extend');
require('is-registered');
require('is-valid-instance');
require('project-name', 'project');
require = fn;

/**
 * Return true if `app` is a valid `Base` instance and base-store is not
 * already registered.
 */

utils.isValid = function(app) {
  if (!utils.isValidInstance(app)) {
    return false;
  }
  if (utils.isRegistered(app, 'base-store')) {
    return false;
  }
  debug('initializing <%s>, from <%s>', __filename, module.parent.id);
  return true;
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
