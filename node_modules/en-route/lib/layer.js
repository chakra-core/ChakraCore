'use strict';

/**
 * Module dependencies.
 */

var debug = require('debug')('router:layer');
var utils = require('./utils');

/**
 * Create a new `Layer` with the given `path`, `options` and callback.
 *
 * @param {String} `path`
 * @param {Object} `options`
 * @param {Function} fn [description]
 */

var Layer = module.exports = function Layer(path, options, fn) {
  if (!(this instanceof Layer)) {
    return new Layer(path, options, fn);
  }

  debug('new %s', path);
  options = options || {};

  this.handle = fn;
  this.name = fn.name || '<unknown>';
  this.regexp = utils.toRegex(path, this.keys = [], options);
  this.params = undefined;
  this.path = undefined;

  if (path === '/' && options.end === false) {
    this.regexp.fast_slash = true;
  }
};

/**
 * Handle the error for the layer.
 *
 * @param {Error} `error`
 * @param {File} `file`
 * @param {function} `next`
 * @api private
 */

Layer.prototype.handleError = function handleError(error, file, next) {
  var fn = this.handle;
  if (fn.length !== 3) {
    // not a standard error handler
    return next(error);
  }

  try {
    fn(error, file, next);
  } catch (err) {
    next(err);
  }
};

/**
 * Handle the file for the layer.
 *
 * @param {File} file
 * @param {function} next
 * @api private
 */

Layer.prototype.handleFile = function handleFile(file, next) {
  var fn = this.handle;
  // not a standard file handler
  if (fn.length > 2) {
    return next();
  }

  try {
    fn(file, next);
  } catch (err) {
    next(err);
  }
};

/**
 * Check if this route matches `path`, if so
 * populate `.params`.
 *
 * @param {String} path
 * @return {Boolean}
 * @api private
 */

Layer.prototype.match = function match(path) {
  if (path == null) {
    // no path, nothing matches
    this.params = undefined;
    this.path = undefined;
    return false;
  }

  // fast path non-ending match for / (everything matches)
  if (this.regexp.fast_slash) {
    this.params = {};
    this.path = '';
    return true;
  }

  var m = this.regexp.exec(path);
  if (!m) {
    this.params = undefined;
    this.path = undefined;
    return false;
  }

  // store values
  this.params = {};
  this.path = m[0];

  var keys = this.keys;
  var params = this.params;
  var prop;
  var n = 0;
  var key;
  var val;

  for (var i = 1, len = m.length; i < len; ++i) {
    key = keys[i - 1];
    prop = key
      ? key.name
      : n++;
    val = utils.decodeParam(m[i]);

    if (val !== undefined || !({}.hasOwnProperty.call(params, prop))) {
      params[prop] = val;
    }
  }

  return true;
};
