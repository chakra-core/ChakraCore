/*!
 * base-cwd <https://github.com/jonschlinkert/base-cwd>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var path = require('path');
var empty = require('empty-dir');
var isValid = require('is-valid-app');
var find = require('find-pkg');

module.exports = function(types, options) {
  if (typeof types !== 'string' && !Array.isArray(types)) {
    options = types;
    types = undefined;
  }

  options = options || {};
  var cached;

  return function plugin(app) {
    if (!isValid(app, 'base-cwd', types)) return;

    var cwds = [app.cwd || process.cwd()];

    Object.defineProperty(this, 'cwd', {
      configurable: true,
      enumerable: true,
      set: function(cwd) {
        cached = app.cache.cwd = path.resolve(cwd);
        if (cwds[cwds.length - 1] !== cached) {
          cwds.push(cached);
          app.emit('cwd', cached);
        }
      },
      get: function() {
        if (typeof cached === 'string') {
          return path.resolve(cached);
        }
        if (typeof app.options.cwd === 'string') {
          return (cached = path.resolve(app.options.cwd));
        }

        var cwd = process.cwd();
        if (options.findup === false) {
          cached = cwd;
          return cwd;
        }

        var isEmpty = empty.sync(cwd, function(fp) {
          return !/\.DS_Store/.test(fp);
        });

        if (isEmpty) {
          cached = cwd;
          return cwd;
        }

        var pkgPath = find.sync(cwd);
        if (pkgPath) {
          var dir = path.dirname(pkgPath);
          if (dir !== cwd) {
            cached = app.cache.cwd = dir;
          }
          return dir;
        }
        return cwd;
      }
    });

    return plugin;
  }
};
