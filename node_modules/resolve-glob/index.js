/*!
 * resolve-glob <https://github.com/jonschlinkert/resolve-glob>
 *
 * Copyright (c) 2015-2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var path = require('path');
var extend = require('extend-shallow');
var isValidGlob = require('is-valid-glob');
var resolveDir = require('resolve-dir');
var relative = require('relative');
var glob = require('matched');

/**
 * async
 */

module.exports = function(patterns, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = {};
  }

  if (typeof cb !== 'function') {
    throw new TypeError('expected callback to be a function');
  }

  if (!isValidGlob(patterns)) {
    cb(new TypeError('expected glob to be a string or array'));
    return;
  }

  var opts = createOptions(options);
  var nonull = opts.nonull === true;
  delete opts.nonull;

  glob(patterns, opts, function(err, files) {
    if (err) {
      cb(err);
      return;
    }

    if (!files.length && nonull) {
      cb(null, arrayify(patterns));
      return;
    }

    cb(null, resolveFiles(files, opts));
  });
};

/**
 * sync
 */

module.exports.sync = function(patterns, options) {
  if (!isValidGlob(patterns)) {
    throw new TypeError('expected glob to be a string or array');
  }

  var opts = createOptions(options);
  var nonull = opts.nonull === true;
  delete opts.nonull;

  var files = glob.sync(patterns, opts);
  if (!files.length && nonull) {
    return arrayify(patterns);
  }
  return resolveFiles(files, opts);
};

/**
 * Utils
 */

function arrayify(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
}

function createOptions(options) {
  var opts = extend({cwd: process.cwd()}, options);
  opts.cwd = path.resolve(resolve(opts.cwd));
  return opts;
}

function resolveFiles(files, opts) {
  var len = files.length, i = -1;
  while (++i < len) {
    files[i] = resolveFile(files[i], opts);
  }
  return files;
}

function resolveFile(fp, opts) {
  fp = path.resolve(opts.cwd, fp);
  if (opts.relative) {
    fp = relative(process.cwd(), fp);
  }
  return fp;
}

function resolve(cwd) {
  return path.resolve(resolveDir(cwd));
}
