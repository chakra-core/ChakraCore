'use strict';

var utils = require('./utils');

module.exports = function(patterns, config) {
  if (!utils.isValidGlob(patterns)) {
    throw new Error('invalid glob pattern: ' + patterns);
  }

  // shallow clone options
  var options = utils.extend({cwd: '', nosort: true}, config);
  options.cwd = utils.cwd(options);
  options.cache = {};

  patterns = utils.arrayify(patterns);
  if (!utils.hasGlob(patterns)) {
    return utils.getPaths(patterns, options);
  }

  var sifted = utils.sift(patterns, options);
  var excludes = sifted.excludes;
  var includes = sifted.includes;
  var Glob = utils.glob.Glob;
  var glob = {cache: {}};
  var cache = [];

  function updateOptions(include) {
    return utils.setIgnores(options, excludes, include.index);
  }

  var len = includes.length, i = -1;
  var files = [];

  while (++i < len) {
    var include = includes[i];
    var opts = updateOptions(include);
    opts.cache = glob.cache;
    opts.sync = true;

    glob = new Glob(include.pattern, opts);
    cache.push(glob.cache);

    files.push.apply(files, glob.found);
  }

  Object.defineProperty(files, 'cache', {
    configurable: true,
    get: function() {
      return utils.createCache(cache);
    }
  });

  return files;
};
