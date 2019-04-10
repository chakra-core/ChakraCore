'use strict';

var path = require('path');
var union = require('arr-union');
var utils = require('lazy-cache')(require);

/**
 * Lazily required module dependencies
 */

var fn = require;
require = utils;
require('glob');
require('async-array-reduce', 'reduce');
require('extend-shallow', 'extend');
require('fs-exists-sync', 'exists');
require('has-glob');
require('is-valid-glob');
require('resolve-dir', 'resolve');
require = fn;

/**
 * utils
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Returns a unique-ified array of all cached filepaths from
 * the `glob.cache` property exposed by [node-glob][].
 *
 * The most time-instensive part of globbing is hitting the
 * file system, so node-glob caches file paths internally to
 * allow multiple matching operations to be performed on the
 * same array - without hitting the file system again. Here, we're
 * simply pushing those cache objects into an array to expose
 * them to the user.
 *
 * @param {Array} `arr` Array of `glob.cache` objects.
 * @return {Array}
 */

utils.createCache = function(arr) {
  return arr.reduce(function(acc, cache) {
    var keys = Object.keys(cache);
    var len = keys.length, i = -1;

    while (++i < len) {
      var key = keys[i];
      var val = cache[key];

      if (Array.isArray(val)) {
        union(acc, val.map(join(key)));
      } else {
        union(acc, [key]);
      }
    }
    return acc;
  }, []);
};

/**
 * Returns a function to join `cwd` to the given file path
 *
 * @param {String} cwd
 * @return {String}
 */

function join(cwd) {
  return function(fp) {
    return path.join(cwd, fp);
  };
}

/**
 * Sift glob patterns into inclusive and exclusive patterns.
 *
 * @param {String|Array} `patterns`
 * @param {Object} opts
 * @return {Object}
 */

utils.sift = function(patterns, opts) {
  patterns = utils.arrayify(patterns);
  var res = { includes: [], excludes: [] };
  var len = patterns.length, i = -1;

  while (++i < len) {
    var stat = new utils.Stat(patterns[i], i);

    if (opts.relative) {
      stat.pattern = utils.toRelative(stat.pattern, opts);
      delete opts.cwd;
    }

    if (stat.isNegated) {
      res.excludes.push(stat);
    } else {
      res.includes.push(stat);
    }
  }
  res.stat = stat;
  return res;
};

/**
 * Set the index of ignore patterns based on their position
 * in an array of globs.
 *
 * @param {Object} `options`
 * @param {Array} `excludes`
 * @param {Number} `inclusiveIndex`
 */

utils.setIgnores = function(options, excludes, inclusiveIndex) {
  var opts = utils.extend({}, options);
  var negations = [];

  var len = excludes.length, i = -1;
  while (++i < len) {
    var exclusion = excludes[i];
    if (exclusion.index > inclusiveIndex) {
      negations.push(exclusion.pattern);
    }
  }
  opts.ignore = utils.arrayify(opts.ignore || []);
  opts.ignore.push.apply(opts.ignore, negations);
  return opts;
};

/**
 * Create a new `Stat` object with information about a
 * single glob pattern.
 *
 * @param {String} `pattern` Glob pattern
 * @param {Number} `idx` Index of the glob in the given array of patterns
 */

utils.Stat = function(pattern, idx) {
  this.index = idx;
  this.isNegated = false;
  this.pattern = pattern;

  if (pattern.charAt(0) === '!') {
    this.isNegated = true;
    this.pattern = pattern.slice(1);
  }
};

/**
 * Resolve the `cwd` to use for a glob operation.
 *
 * @param {Object} `options`
 * @return {String}
 */

utils.cwd = function(options) {
  var cwd = options.cwd;
  if (/^\W/.test(cwd)) {
    cwd = utils.resolve(cwd);
  }
  return path.resolve(cwd);
};

/**
 * Make a glob pattern relative.
 *
 * @param {String} `pattern`
 * @param {Object} `opts`
 * @return {String}
 */

utils.toRelative = function(pattern, opts) {
  var fp = path.resolve(opts.cwd, pattern);
  return path.relative(process.cwd(), fp);
};

/**
 * Get paths from non-glob patterns
 *
 * @param {Array} `paths`
 * @param {Object} `opts`
 * @return {Array}
 */

utils.getPaths = function(paths, opts) {
  paths = paths.filter(function(fp) {
    return utils.exists(path.resolve(opts.cwd, fp));
  });
  if (opts.realpath) {
    paths = paths.map(function(fp) {
      return path.resolve(opts.cwd, fp);
    });
  }
  return paths;
};

/**
 * Expose `utils`
 */

module.exports = utils;
