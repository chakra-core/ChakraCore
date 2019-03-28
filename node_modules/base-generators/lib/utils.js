'use strict';

var fs = require('fs');
var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils; // eslint-disable-line

/**
 * Utils
 */

require('async-each-series', 'eachSeries');
require('base-compose', 'compose');
require('base-cwd', 'cwd');
require('base-data', 'data');
require('base-env', 'env');
require('base-option', 'option');
require('base-pkg', 'pkg');
require('base-plugins', 'plugins');
require('base-task', 'task');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('global-modules', 'gm');
require('is-valid-app', 'isValid');
require('is-valid-instance');
require('kind-of', 'typeOf');
require('mixin-deep', 'merge');
require = fn; // eslint-disable-line

/**
 * Cache lookups
 */

var lookupCache = {};

/**
 * Returns true if a task or array of tasks is valid
 */

utils.isValidTasks = function(val) {
  if (!val) return false;
  if (utils.isString(val)) {
    return !/\W/.test(val);
  }
  if (!Array.isArray(val)) {
    return false;
  }
  return val.every(function(str) {
    return utils.isString(str) && !/\W/.test(str);
  });
};

utils.isGenerator = function(val) {
  return utils.isApp(val, 'Generator');
};

utils.isApp = function(val, ctorName) {
  return utils.isObject(val) && val['is' + ctorName] === true;
};

/**
 * Return true if `val` is a string with length greater than zero.
 */

utils.isString = function(val) {
  return val && typeof val === 'string';
};

/**
 * Create an array of tasks
 */

utils.toArray = function(val) {
  if (Array.isArray(val)) {
    return val.reduce(function(acc, str) {
      return acc.concat(utils.toArray(str));
    }, []);
  }
  if (utils.isString(val)) {
    return val.split(',');
  }
  return [];
};

/**
 * Cast `val` to an array
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Returns true if `val` is an object
 */

utils.isObject = function(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Try to require a module `name` locally, from local node_modules,
 * then global npm directory
 */

utils.tryRequire = function(name, cwd) {
  cwd = cwd || process.cwd();
  try {
    return require(name);
  } catch (err) {}

  try {
    return require(path.resolve(name));
  } catch (err) {}

  try {
    return require(path.resolve(cwd, 'node_modules', name));
  } catch (err) {}

  try {
    return require(path.resolve(utils.gm, name));
  } catch (err) {}
  return null;
};

/**
 * Return true if `filepath` exists on the file system
 */

utils.exists = function(name) {
  if (lookupCache.hasOwnProperty(name)) {
    return lookupCache[name];
  }

  function set(name, fp) {
    if (name) lookupCache[name] = true;
    if (fp) lookupCache[fp] = true;
  }

  try {
    fs.lstatSync(name);
    set(name);
    return true;
  } catch (err) {};

  try {
    var fp = path.resolve('node_modules', name);
    if (lookupCache[fp]) return true;

    fs.lstatSync(fp);
    set(name, fp);
    return true;
  } catch (err) {}

  try {
    fp = path.resolve(utils.gm, name);
    if (lookupCache[fp]) return true;
    fs.lstatSync(fp);
    set(name, fp);
    return true;
  } catch (err) {}

  lookupCache[name] = false;
  return false;
};

/**
 * Handle generator errors
 */

utils.handleError = function(app, name, next) {
  return function(err) {
    if (!err) {
      next();
      return;
    }

    var msg = utils.formatError(err, app, name);
    if (!msg) {
      next();
      return;
    }

    if (msg instanceof Error) {
      next(err);
      return;
    }

    next(new Error(msg));
  };
};

utils.formatError = function(err, app, name) {
  if (err instanceof Error) {
    var match = /task "(.*?)" is not registered/.exec(err.message);
    if (!match) {
      return err;
    }

    var taskname = match[1];
    if (taskname === 'default') {
      return;
    }

    if (~name.indexOf(':')) {
      var segs = name.split(':');
      taskname = segs[1];
      name = segs[0];
    }
  }

  var msg = 'Cannot find ';
  var gen = app.getGenerator(name);
  if (gen && name !== taskname) {
    msg += 'task: "' + taskname + (name !== 'this' ? '" in generator' : '');
  } else {
    // don't error when a `default` generator is not defined
    if (name === 'default') {
      return;
    }
    msg += 'generator';
  }

  msg += (name !== 'this' ? ': "' + name + '"' : '');

  var cwd = app.get('options.cwd');
  if (cwd) msg += ' in "' + cwd + '"';
  return msg;
};

/**
 * Expose `utils`
 */

module.exports = utils;
