/*!
 * resolve-up <https://github.com/jonschlinkert/resolve-up>
 *
 * Copyright (c) 2015, 2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var path = require('path');
var isValidGlob = require('is-valid-glob');
var extend = require('extend-shallow');
var unique = require('array-unique');
var paths = require('npm-paths');
var glob = require('matched');
var cache = {};

/**
 * Return a list of directories that match the given filepath/glob
 * patterns, optionally using a pre-filter function before matching
 * globs against paths.
 *
 * ```js
 * var resolveUp = require('resolve-up');
 * console.log(resolveUp('generate-*'));
 * ```
 * @param  {Array|String} `pattern`
 * @param  {Function} `fn` Optional pre-filter function
 * @return {Array}
 * @api public
 */

function resolveUp(patterns, options) {
  if (!isValidGlob(patterns)) {
    throw new Error('resolve-up expects a string or array as the first argument.');
  }

  var opts = extend({fast: true}, options);
  var dirs = paths(opts).concat(opts.paths || []);
  var len = dirs.length;
  var idx = -1;
  var res = [];

  while (++idx < len) {
    opts.cwd = dirs[idx];
    if (!opts.cwd) continue;
    opts.cwd = path.resolve(opts.cwd);
    var key = opts.cwd + '=' + patterns;
    if (cache[key]) {
      res.push.apply(res, cache[key]);
    } else {
      var files = resolve(glob.sync(patterns, opts), opts);
      cache[key] = files;
      res.push.apply(res, files);
    }
  }
  return unique(res);
};

/**
 * Resolve the absolute path to a file using the given cwd.
 */

function resolve(files, opts) {
  var fn = opts.filter || function(fp) {
    return true;
  };

  if (opts.realpath === true) {
    return files.filter(fn);
  }

  var len = files.length;
  var idx = -1;
  var res = [];

  while (++idx < len) {
    var fp = path.resolve(opts.cwd, files[idx]);
    if (!fn(fp) || ~res.indexOf(fp)) continue;
    res.push(fp);
  }
  return res;
}

/**
 * Expose `resolveUp`
 */

module.exports = resolveUp;
