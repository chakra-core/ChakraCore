'use strict';

var typeOf = require('kind-of');
var extend = require('extend-shallow');
var utils = require('./lib/utils');

function normalize(config, dest, options) {
  if (arguments.length > 1) {
    config = toObject(config, dest, options);
  }

  var res = null;
  switch(typeOf(config)) {
    case 'string':
      res = normalizeString(config);
      break;
    case 'object':
      res = normalizeObject(config);
      break;
    case 'array':
      res = normalizeArray(config);
      break;
    default: {
      throw new TypeError('unsupported argument type: ' + config);
    }
  }

  var thisArg = {};
  if (utils.isObject(this) && this.options) {
    thisArg = this.options;
  }

  // copy `thisArg` options onto config root
  copyOptions(res, thisArg);
  return formatObject(res);
}

/**
 * Move reserved options properties from `obj` to `obj.options`
 */

function normalizeOptions(obj) {
  for (var key in obj) {
    if (utils.optsKeys.indexOf(key) > -1) {
      obj.options = obj.options || {};
      obj.options[key] = obj[key];
      delete obj[key];
    }
  }
  return obj;
}

/**
 * Copy options
 */

function copyOptions(config, thisArg) {
  var ctx = thisArg.options || thisArg;
  config.options = extend({}, ctx, config.options);
  return config;
}

/**
 * Convert args list to a config object.
 */

function toObject(src, dest, options) {
  if (!utils.isObject(options)) {
    options = {};
  }

  var config = {};
  if (utils.isObject(dest)) {
    options = extend({}, options, dest);
    dest = null;
  }

  if (utils.isObject(src)) {
    config = src;
  }

  if (isValidSrc(src)) {
    config.src = src;
  } else if (Array.isArray(src)) {
    config = normalize(src);
  }

  if (typeof dest === 'string') {
    config.dest = dest;
  }

  if (options.hasOwnProperty('options')) {
    options = extend({}, options, options.options);
    delete options.options;
  }

  config.options = extend({}, config.options, options);
  return config;
}

/**
 * Object
 */

function normalizeObject(val) {
  if (val.hasOwnProperty('path')) {
    val.src = val.path;
  }

  val = normalizeOptions(val);

  if (val.options && val.options.files) {
    var config = {};
    config.files = val.options.files;
    delete val.options.files;
    return normalize(config, val);
  }

  // if src/dest are on options, move them to root
  val = utils.move(val, 'src');
  val = utils.move(val, 'dest');

  // allow src to be a getter
  if ('src' in val || val.hasOwnProperty('dest')) {
    return toFiles(val);
  }

  if (!val.hasOwnProperty('files')) {
    return filesObjects(val);
  }

  if (Array.isArray(val.files)) {
    return reduceFiles(val.files, val);
  }

  if (val.files && !utils.isObject(val.files)) {
    throw new TypeError('expected "files" to be an array or object');
  }

  val.files = normalizeFiles(val);
  return val;
}

/**
 * Ensure that `src` on the given val is an array
 *
 * @param {Object} `val` Object with a `src` property
 * @return {Object}
 */

function normalizeSrc(val) {
  if (!val.src) return val;
  val.src = utils.arrayify(val.src);
  return val;
}

/**
 * String
 */

function normalizeString(val) {
  return toFiles({src: [val]});
}

/**
 * Array
 */

function normalizeArray(arr) {
  if (isValidSrc(arr[0])) {
    return toFiles({src: arr});
  }
  return reduceFiles(arr);
}

/**
 * Files property
 */

function normalizeFiles(val) {
  var res = normalize(val.files);
  return res.files;
}

/**
 * Normalize all objects in a `files` array.
 *
 * @param {Array} `files`
 * @return {Array}
 */

function reduceFiles(files, orig) {
  var config = {files: []};
  var len = files.length;
  var idx = -1;

  while (++idx < len) {
    var val = normalize(files[idx]);
    config.files = config.files.concat(val.files);
  }

  return copyNonfiles(config, orig);
}

/**
 * Create a `files` array from a src-dest object.
 *
 * ```js
 * // converts from:
 * { src: '*.js', dest: 'foo/' }
 *
 * // to:
 * { files: [{ src: ['*.js'], dest: 'foo/' }] }
 * ```
 */

function copyNonfiles(config, provider) {
  if (!provider) return config;
  for (var key in provider) {
    if (!isFilesKey(key)) {
      config[key] = provider[key];
    }
  }
  return config;
}

/**
 * Create a `files` array from a src-dest object.
 *
 * ```js
 * // converts from:
 * { src: '*.js', dest: 'foo/' }
 *
 * // to:
 * { files: [{ src: ['*.js'], dest: 'foo/' }] }
 * ```
 */

function toFiles(val) {
  var config = {files: [normalizeSrc(val)]};
  return copyNonfiles(config, val);
}

/**
 * When `src`, `dest` and `files` are absent from the
 * object, we check to see if file objects were defined.
 *
 * ```js
 * // converts from:
 * { 'foo/': '*.js' }
 *
 * // to
 * { files: [{ src: ['*.js'], dest: 'foo/' }] }
 * ```
 */

function filesObjects(val) {
  var res = {};
  if (val.options) res.options = val.options;
  res.files = [];

  for (var key in val) {
    if (key !== 'options') {
      var file = {};
      if (val.options) file.options = val.options;
      file.src = utils.arrayify(val[key]);
      file.dest = key;
      res.files.push(file);
    }
  }
  return res;
}

/**
 * Optionally sort the keys in all of the files objects.
 * Helps with debugging.
 *
 * @param {Object} `val` Pass `{sort: true}` on `val.options` to enable sorting.
 * @return {Object}
 */

function formatObject(val) {
  if (val.options && val.options.format === false) {
    return val;
  }

  var res = { options: val.options };
  res.files = val.files;
  for (var key in val) {
    if (key === 'files' || key === 'options') {
      continue;
    }
    res[key] = val[key];
  }

  var len = res.files.length;
  var idx = -1;

  while (++idx < len) {
    var ele = res.files[idx];
    var obj = {};

    obj.options = {};
    obj.src = ele.src || [];
    obj.dest = ele.dest || '';

    copyNonfiles(obj, ele);
    if (!ele.options) {
      obj.options = res.options;
    }

    res.files[idx] = obj;
  }
  return res;
}

/**
 * Boolean checks
 */

function isFilesKey(key) {
  return ['src', 'dest', 'files'].indexOf(key) > -1;
}

function isValidSrc(val) {
  if (typeof val === 'string') {
    return true;
  }
  if (Array.isArray(val)) {
    return val[0].src || val[0].dest;
  }
  return false;
}

/**
 * Expose `normalize`
 */

module.exports = normalize;
