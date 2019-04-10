'use strict';

var path = require('path');
var isWindows = require('is-windows');
var gm = require('global-modules');

/**
 * Returns an array of directories to use as a starting
 * point for lookups.
 *
 * @param  {Object} `options`
 * @option {String} `options.cwd` The path to search from. Defaults to `process.cwd()`.
 * @option {String} `options.module` The name of a module to append to each path in the returned array of paths. The advantage of using this is that you won't have to loop over the paths again.
 * @option {Boolean} `options.fast` Returns the resolved path to `node_modules` relative to the cwd and the `node_modules` relative to the global modules paths used by npm, and **skips all intermediate directories**. Defaults to `undefined`.
 * @return {Array} Array of directories
 */

module.exports = function npmPaths(options) {
  options = options || {};
  var cwd = options.cwd || process.cwd();

  var segs = cwd.split(path.sep);
  var paths = ['/node_modules'];
  var addPath = union(paths, options);

  var modulePath = typeof options.module === 'string'
    ? options.module
    : null;

  if (options.fast === true) {
    addPath(npm(cwd, modulePath));
  } else {
    while (segs.pop()) {
      var fp = npm(segs.join(path.sep), modulePath);
      if (fp.charAt(0) !== '/' && !isWindows()) {
        fp = '/' + fp;
      }
      addPath(fp);
    }
  }

  if (process.env.NODE_PATH) {
    process.env.NODE_PATH
      .split(path.delimiter)
      .filter(Boolean)
      .forEach(addPath);
  } else {
    addPath(path.join(gm, modulePath || ''));
    if (isWindows()) {
      addPath(path.join(npm(process.env.APPDATA), 'npm'));
    } else {
      addPath(npm('/usr/lib', modulePath));
    }
  }

  paths.sort(function(a, b) {
    return a.split(path.sep).length < b.split(path.sep).length;
  });
  return paths;
};

function union(paths, options) {
  return function(filepath) {
    if (typeof options.filter === 'function') {
      if (!options.filter(filepath)) return;
    }
    if (paths.indexOf(filepath) === -1) {
      paths.push(filepath);
    }
  };
}

function npm(dir, name) {
  return path.resolve(dir, 'node_modules', name || '');
}
