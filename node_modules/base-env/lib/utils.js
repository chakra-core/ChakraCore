'use strict';

var path = require('path');
var debug = require('debug')('base:env');
var utils = module.exports = {};
var requireCache = {};

utils.namespace = require('base-namespace');
utils.containsPath = require('contains-path');
utils.extend = require('extend-shallow');
utils.exists = require('fs-exists-sync');
utils.gm = require('global-modules');
utils.isAbsolute = require('is-absolute');
utils.isValidApp = require('is-valid-app');
utils.isValidInstance = require('is-valid-instance');
utils.typeOf = require('kind-of');
utils.home = require('os-homedir');
utils.resolve = require('resolve-file');

/**
 * Return true if val is an object
 */

utils.isObject = function(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Return true if val is a string with length > 0.
 */

utils.isString = function(val) {
  return val && typeof val === 'string';
};

/**
 * Return the given value unchanged
 */

utils.identity = function(val) {
  return val;
};

/**
 * Returns true if `app` is a valid instance of `Base`
 */

utils.isValid = function(app) {
  if (!utils.isValidApp(app, 'base-env')) {
    return false;
  }
  debug('initializing <%s>, called from <%s>', __filename, module.parent.id);
  return true;
};

/**
 * Return true if val is an object or appears to be a valid env arg.
 */

utils.isValidArg = function(val) {
  return utils.isAppArg(val) || utils.isObject(val);
};

/**
 * Returns true if `val` appears to be, or might resolve to, a valid instance of Base.
 */

utils.isAppArg = function(val) {
  return utils.isValidInstance(val)
    || typeof val === 'function'
    || typeof val === 'string';
};

utils.tryResolve = function(name) {
  if (requireCache[name]) {
    return requireCache[name];
  }

  try {
    var fp = path.resolve(process.cwd(), 'node_modules', name);
    var res = require.resolve(fp);
    requireCache[name] = res;
    return res;
  } catch (err) {}
};

/**
 * Inspect utils
 */

utils.instanceName = function(env) {
  var name = env.app && (env.app.namespace || env.app._namespace || env.app._name);
  return 'instance' + (name ? (' ' + name) : '');
};

utils.functionName = function(env) {
  return 'function' + (env.fn.name ? (' ' + env.fn.name) : '');
};

utils.pathName = function(env) {
  var fp = env.key;
  if (utils.exists(fp) && !utils.containsPath(process.cwd(), env.path)) {
    fp = path.relative(utils.home(), path.resolve(env.path));
  }
  return 'path ~/' + fp;
};
