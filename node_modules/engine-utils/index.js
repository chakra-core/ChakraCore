/**
 * Utilities from conslidate.js
 *
 */

'use strict';

/**
 * Module dependencies.
 */

var fs = require('fs');
var path = require('path');

/**
 * Requires cache.
 */

exports.requires = {};

/**
 * File/string caches
 */

var cacheStore = {};
var readCache = {};

/**
 * Clear the cache.
 *
 * @api public
 */

exports.clearCache = function () {
  cacheStore = {};
};

/**
 * Conditionally cache `compiled` template based
 * on the `options` filename and `.cache` boolean.
 *
 * @param {Object} options
 * @param {Function} compiled
 * @return {Function}
 * @api private
 */

exports.cache = function cache(options, compiled) {
  // cachable
  if (compiled && options.filename && options.cache) {
    delete readCache[options.filename];
    cacheStore[options.filename] = compiled;
    return compiled;
  }

  // check cache
  if (options.filename && options.cache) {
    return cacheStore[options.filename];
  }
  return compiled;
};

/**
 * Read `path` with `options` with
 * callback `(err, str)`. When `options.cache`
 * is true the template string will be cached.
 *
 * @param {String} options
 * @param {Function} callback
 * @api private
 */

function read(path, options, callback) {
  var str = readCache[path];
  var cached = options.cache && str && ('string' === typeof str);

  // cached (only if cached is a string and not a compiled template function)
  if (cached) {
    return callback(null, str);
  }

  // read
  fs.readFile(path, 'utf8', function (err, str) {
    if (err) {
      return callback(err);
    }
    // remove extraneous utf8 BOM marker
    str = str.replace(/^\uFEFF/, '');
    if (options.cache) {readCache[path] = str;}
    callback(null, str);
  });
}


/**
 * Read `path` with `options` with
 * callback `(err, str)`. When `options.cache`
 * is true the partial string will be cached.
 *
 * @param {String} options
 * @param {Function} callback
 * @api private
 */

exports.readPartials = function(filepath, options, callback) {
  if (!options.partials) {
    return callback();
  }
  var partials = options.partials;
  var keys = Object.keys(partials);

  function next(index) {
    if (index === keys.length) {
      return callback(null);
    }
    var key = keys[index];
    var file = path.join(path.dirname(filepath), partials[key] + path.extname(filepath));
    read(file, options, function (err, str) {
      if (err) {
        return callback(err);
      }
      options.partials[key] = str;
      next(++index);
    });
  }
  next(0);
};

/**
 * fromStringRenderer
 */

exports.fromStringRenderer = function (name) {
  return function (path, options, callback) {
    options.filename = path;
    exports.readPartials(path, options, function (err) {
      if (err) {
        return callback(err);
      }
      if (exports.cache(options)) {
        exports[name].render('', options, callback);
      } else {
        read(path, options, function (err, str) {
          if (err) {
            return callback(err);
          }
          exports[name].render(str, options, callback);
        });
      }
    });
  };
};