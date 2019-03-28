'use strict';

var debug = require('debug')('base:helpers');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('isobject', 'isObject');
require('define-property', 'define');
require('is-valid-app');
require('load-helpers', 'loader');
require = fn;

/**
 * Return false if `app` is not a valid instance of `Base`, or
 * the `base-helpers` plugin is alread registered.
 */

utils.isValid = function(app) {
  if (utils.isValidApp(app, 'base-helpers', ['app', 'views', 'collection'])) {
    debug('initializing <%s>, from <%s>', __filename, module.parent.id);
    return true;
  }
  return false;
};
/**
 * Return true if the given value is a helper "group"
 */

utils.isHelperGroup = function(helpers) {
  if (!helpers) return false;
  if (typeof helpers === 'function' || utils.isObject(helpers)) {
    var len = Object.keys(helpers).length;
    var min = helpers.async ? 1 : 0;
    return helpers.isGroup === true || len > min;
  }
  if (Array.isArray(helpers)) {
    return helpers.isGroup === true;
  }
  return false;
};

/**
 * Arrayify the given value by casting it to an array.
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
