'use strict';

var utils = require('./utils');

module.exports = function(patterns, config, cb) {
  if (typeof config === 'function') {
    cb = config;
    config = {};
  }

  if (typeof cb !== 'function') {
    throw new Error('expected a callback function.');
  }

  if (!utils.isValidGlob(patterns)) {
    cb(new Error('invalid glob pattern: ' + patterns));
    return;
  }

  // shallow clone options
  var options = utils.extend({cwd: ''}, config);
  options.cwd = utils.cwd(options);

  patterns = utils.arrayify(patterns);
  if (!utils.hasGlob(patterns)) {
    patterns = utils.getPaths(patterns, options);
    cb(null, patterns);
    return;
  }

  var sifted = utils.sift(patterns, options);
  var Glob = utils.glob.Glob;
  var cache = [];
  var glob;

  function updateOptions(inclusive) {
    return utils.setIgnores(options, sifted.excludes, inclusive.index);
  }

  utils.reduce(sifted.includes, [], function(acc, include, next) {
    var opts = updateOptions(include);
    if (acc.glob) {
      opts.cache = acc.glob.cache;
    }

    glob = new Glob(include.pattern, opts, function(err, files) {
      if (err) {
        next(err);
        return;
      }

      cache.push(glob.cache);
      acc = acc.concat(files);
      acc.glob = glob;
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
