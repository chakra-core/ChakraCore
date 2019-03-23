/*!
 * is-registered (https://github.com/jonschlinkert/is-registered)
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var define = require('define-property');
var isObject = require('isobject');

module.exports = function(app, name) {
  if (!isObject(app) || typeof name !== 'string') {
    return true;
  }

  if (typeof app.isRegistered !== 'function') {
    define(app, 'registered', {});
    define(app, 'isRegistered', function(name) {
      if (app.registered.hasOwnProperty(name)) {
        return true;
      }
      app.registered[name] = true;
      return false;
    });
  }

  return app.isRegistered(name);
};
