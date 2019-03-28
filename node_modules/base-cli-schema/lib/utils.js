'use strict';

var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('arr-flatten', 'flatten');
require('array-unique', 'unique');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('fs-exists-sync', 'exists');
require('falsey');
require('has-glob');
require('has-value');
require('kind-of', 'typeOf');
require('map-schema', 'Schema');
require('mixin-deep', 'merge');
require('resolve');
require('tableize-object', 'tableize');
require = fn;

utils.tryResolve = function(name, cwd) {
  if (!/\.js$/.test(name)) {
    name += '.js';
  }

  if (typeof cwd === 'string') {
    try {
      if (!utils.exists(cwd)) {
        return;
      }
      var fp = path.resolve(cwd, name);
      return require.resolve(fp);
    } catch (err) {}
  }

  try {
    return require.resolve(name);
  } catch (err) {}

  try {
    return require.resolve(path.resolve(name));
  } catch (err) {}
};

/**
 * Cast `val` to an array.
 */

utils.arrayify = function(val) {
  if (utils.isString(val)) {
    val = val.split(',');
  }
  if (Array.isArray(val)) {
    return val.filter(Boolean);
  }
  return [];
};

/**
 * Strip the quotes from a string
 */

utils.stripQuotes = function(str) {
  return str.replace(/^['"]|['"]$/g, '');
};

/**
 * Return true if value is false, undefined, null, an empty array
 * or empty object.
 */

utils.isEmpty = function(val) {
  if (typeof val === 'function') {
    return false;
  }
  return !utils.hasValue(val);
};

/**
 * Return true if value is an object
 */

utils.isObject = function(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Return true if value is a string with non-zero length
 */

utils.isString = function(val) {
  return val && typeof val === 'string';
};

/**
 * Arrayify, flatten and uniquify `arr`
 */

utils.unify = function(arr) {
  return utils.flatten(utils.arrayify(arr));
};

/**
 * Stringify `obj` by tableizing keys and key-value pairs.
 */

utils.stringify = function(obj) {
  obj = utils.tableize(obj);
  var res = '';
  for (var prop in obj) {
    res += prop + '.' + obj[prop];
  }
  return res;
};

/**
 * Invert the keys and values in `obj`.
 */

utils.invert = function invert(obj) {
  var res = {};
  for (var key in obj) res[obj[key]] = key;
  return res;
};

/**
 * File-related properties. Passed to [expand-args]
 * to ensure that no undesired escaping or processing
 * is done on filepath arguments.
 */

utils.fileKeys = [
  'base', 'basename', 'cwd', 'dir',
  'dirname', 'ext', 'extname', 'f',
  'file', 'filename', 'path', 'root',
  'stem'
];

/**
 * Whitelisted flags: these flags can be passed along with task
 * arguments. To run tasks with any flags, pass `--run`
 */

utils.whitelist = [
  'ask',
  'data',
  'emit',
  'force',
  'init',
  'layout',
  'option',
  'options',
  'readme',
  'set',
  'toc',
  'verbose'
].concat(utils.fileKeys);

/**
 * Aliases to pass to minimist. Defined here
 * so that it's available to any method.
 */

utils.aliases = {
  dirname: 'dir',
  extname: 'ext',
  file: 'f',
  filename: 'stem',
  global: 'g',
  save: 's',
  verbose: 'v',
  version: 'V'
};

/**
 * Expose `utils`
 */

module.exports = utils;

