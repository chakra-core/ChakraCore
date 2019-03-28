'use strict';

var fs = require('fs');
var path = require('path');

/**
 * Lazily required module dependencies
 */

var utils = require('lazy-cache')(require);
var fn = require;

require = utils;
require('arr-flatten', 'flatten');
require('extend-shallow', 'extend');
require('get-value', 'get');
require('has-glob');
require('has-value', 'has');
require('is-registered');
require('is-valid-app');
require('kind-of', 'typeOf');
require('merge-value');
require('mixin-deep', 'merge');
require('read-file', 'read');
require('resolve-glob', 'resolve');
require('set-value', 'set');
require('union-value');
require = fn;

/**
 * Utils
 */

utils.isValid = function(app, prop) {
  return utils.isValidApp(app, 'base-data:' + prop, ['app', 'collection', 'list', 'views', 'group']);
};

/**
 * Return the last item in `arr`
 */

utils.last = function(arr) {
  if (!Array.isArray(arr)) {
    throw new TypeError('expected value to be an array');
  }
  return arr[arr.length - 1];
};

/**
 * Cast `val` to an array
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Iterate over registered data loaders and return any
 * that match the given `filepath`
 *
 * @param {Array} `loaders`
 * @param {String} `filepath`
 * @return {Array} Returns an array of data-loader functions
 */

utils.matchLoaders = function(loaders, fp) {
  var len = loaders.length, i = -1;
  var ext = path.extname(fp);
  var fns = [];

  if (len === 0 && ext === '.json') {
    return [function(str) {
      return JSON.parse(str);
    }];
  }

  while (++i < len) {
    var loader = loaders[i];
    var name = loader.name;
    if (typeof name === 'string' && ext === utils.formatExt(name)) {
      fns.push(loader.fn);

    } else if (utils.typeOf(name) === 'regexp' && name.test(ext)) {
      fns.push(loader.fn);
    }
  }
  if (!fns.length) return null;
  return fns;
};

/**
 * format the given file extension to always start with a dot
 */

utils.formatExt = function(ext) {
  if (ext.charAt(0) !== '.') {
    return '.' + ext;
  }
  return ext;
};

/**
 * Namespace a file
 */

utils.namespace = function(key, data, opts) {
  var stem = utils.rename(key, data, opts);
  if (opts.namespace === true && stem === 'data') {
    return data;
  }

  var obj = {};
  utils.set(obj, stem, data);
  return obj;
};

/**
 * Rename a file
 */

utils.rename = function(key, data, opts) {
  var renameFn = utils.basename;
  if (typeof opts.namespace === 'string') {
    return opts.namespace;
  }
  if (typeof opts.namespace === 'function') {
    renameFn = opts.namespace;
  }
  return renameFn(key, data, opts);
};

/**
 * Get the basename of a filepath excluding extension.
 * This is used as the default renaming function
 * when `namespace` is true.
 */

utils.basename = function(fp) {
  return path.basename(fp, path.extname(fp));
};

/**
 * Return true if the key/value pair looks like a glob
 * + options or undefined.
 */

utils.isGlob = function(key, val) {
  if (typeof val === 'undefined' || utils.isObject(val)) {
    return utils.hasGlob(key);
  }
  return false;
};

/**
 * Return true if an object has any of the given keys.
 * @return {Boolean}
 */

utils.hasAny = function(obj, keys) {
  var len = keys.length;
  while (len--) {
    if (obj.hasOwnProperty(keys[len])) {
      return true;
    }
  }
  return false;
};

/**
 * Return true if the given value is an object.
 * @return {Boolean}
 */

utils.isObject = function(val) {
  if (!val || Array.isArray(val)) {
    return false;
  }
  return typeof val === 'object';
};

/**
 * Return true if the given value looks like an options
 * object.
 */

var optsKeys = ['namespace', 'renameKey'];
var globKeys = [
  'base',
  'cwd',
  'destBase',
  'expand',
  'ext',
  'extDot',
  'extend',
  'flatten',
  'rename',
  'process',
  'srcBase'
];

utils.isOptions = function(val) {
  if (!utils.isObject(val)) return false;
  var keys = globKeys.concat(optsKeys);
  return utils.hasAny(val, keys);
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
