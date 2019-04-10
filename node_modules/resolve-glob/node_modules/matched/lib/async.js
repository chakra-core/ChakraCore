'use strict';

var glob = require('glob');
var reduce = require('async-array-reduce');
var isValidGlob = require('is-valid-glob');
var hasGlob = require('has-glob');
var utils = require('./utils');

module.exports = function(patterns, config, cb) {
  if (typeof config === 'function') {
    cb = config;
    config = {};
  }

  if (typeof cb !== 'function') {
    throw new Error('expected a callback function.');
  }

  if (!isValidGlob(patterns)) {
    cb(new Error('invalid glob pattern: ' + patterns));
    return;
  }

  // shallow clone options
  var options = Object.assign({cwd: ''}, config);
  options.cwd = utils.cwd(options);

  patterns = utils.arrayify(patterns);
  if (!hasGlob(patterns)) {
    patterns = utils.getPaths(patterns, options);
    cb(null, patterns);
    return;
  }

  var sifted = utils.sift(patterns, options);
  var Glob = glob.Glob;
  var cache = [];
  var res;

  function updateOptions(inclusive) {
    return utils.setIgnores(options, sifted.excludes, inclusive.index);
  }

  reduce(sifted.includes, [], function(acc, include, next) {
    var opts = updateOptions(include);
    if (acc.glob) {
      opts.cache = acc.glob.cache;
    }

    res = new Glob(include.pattern, opts, function(err, files) {
      if (err) {
        next(err);
        return;
      }

      cache.push(res.cache);
      acc = acc.concat(files);
      acc.glob = res;
      next(null, acc);
    });
  }, function(err, files) {
    if (err) {
      cb(err);
      return;
    }

    Object.defineProperty(files, 'cache', {
      configurable: true,
      get: function() {
        return utils.createCache(cache);
      }
    });

    delete files.glob;
    cb(null, files);
  });
};
