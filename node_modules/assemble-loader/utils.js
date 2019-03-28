'use strict';

var utils = require('lazy-cache')(require);

/**
 * Lazily required module dependencies
 */

var fn = require;
require = utils;
require('extend-shallow', 'extend');
require('file-contents', 'contents');
require('is-registered');
require('is-valid-glob');
require('is-valid-instance');
require('isobject', 'isObject');
require('load-templates', 'Loader');
require = fn;

/**
 * Return true if app is a valid instance
 */

utils.isValid = function(app) {
  if (!utils.isValidInstance(app)) {
    return false;
  }
  if (utils.isRegistered(app, 'assemble-loader')) {
    return false;
  }
  return true;
};

/**
 * Return true if obj is a View instance
 */

utils.isView = function(obj) {
  return utils.isObject(obj) && (obj.path || obj.contents || obj.isView || obj.isItem);
};

/**
 * Expose utils
 */

module.exports = utils;
