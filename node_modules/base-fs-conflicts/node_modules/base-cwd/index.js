/*!
 * base-cwd <https://github.com/jonschlinkert/base-cwd>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var path = require('path');
var isValid = require('is-valid-app');

module.exports = function(types) {
  return function plugin(app) {
    if (!isValid(app, 'base-cwd', types)) return;

    Object.defineProperty(this, 'cwd', {
      configurable: true,
      enumerable: true,
      set: function(cwd) {
        this.cache.cwd = path.resolve(cwd);
      },
      get: function() {
        if (typeof this.cache.cwd === 'string') {
          return path.resolve(this.cache.cwd);
        }
        if (typeof this.options.cwd === 'string') {
          return path.resolve(this.options.cwd);
        }
        return process.cwd();
      }
    });

    return plugin;
  }
};
