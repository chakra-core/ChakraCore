/*!
 * base-project <https://github.com/jonschlinkert/base-project>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isValidInstance = require('is-valid-instance');
var isRegistered = require('is-registered');
var project = require('project-name');

module.exports = function(fn) {
  return function(app) {
    if (!isValid(app, fn)) return;

    var name;
    this.define('project', {
      configurable: true,
      enumerable: true,
      set: function(val) {
        name = val;
      },
      get: function() {
        return name || (name = project(this.cwd));
      }
    });
  };
};

function isValid(app, fn) {
  if (typeof fn === 'function') {
    return fn(app);
  }
  if (!isValidInstance(app)) {
    return false;
  }
  if (isRegistered(app, 'base-project')) {
    return false;
  }
  return true;
}
