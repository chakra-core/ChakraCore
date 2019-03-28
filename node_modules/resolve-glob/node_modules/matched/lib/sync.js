'use strict';

var glob = require('glob');
var hasGlob = require('has-glob');
var isValidGlob = require('is-valid-glob');
var utils = require('./utils');

module.exports = function(patterns, config) {
  if (!isValidGlob(patterns)) {
    throw new Error('invalid glob pattern: ' + patterns);
  }

  // shallow clone options
  var options = Object.assign({cwd: '', nosort: true}, config);
  options.cwd = utils.cwd(options);
  options.cache = {};

  patterns = utils.arrayify(patterns);
  if (!hasGlob(patterns)) {
    return utils.getPaths(patterns, options);
  }

  var Glob = glob.Glob;
  var sifted = utils.sift(patterns, options);
  var excludes = sifted.excludes;
  var includes = sifted.includes;
  var res = { cache: {} };
  var cache = [];

  function updateOptions(include) {
    return utils.setIgnores(options, excludes, include.index);
  }

  var len = includes.length;
  var idx = -1;
  var files = [];

  while (++idx < len) {
    var include = includes[idx];
    var opts = updateOptions(include);
    opts.cache = res.cache;
    opts.sync = true;

    res = new Glob(include.pattern, opts);
    cache.push(res.cache);

    files.push.apply(files, res.found);
  }

  Object.defineProperty(files, 'cache', {
    configurable: true,
    get: function() {
      return utils.createCache(cache);
    }
  });

  return files;
};
