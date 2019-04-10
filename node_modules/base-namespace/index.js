'use strict';

var isValid = require('is-valid-app');

/**
 * Decorate a `namespace` getter onto `app`
 */

module.exports = function() {
  return function(app) {
    if (!isValid(app, 'base-namespace')) return;

    Object.defineProperty(app, 'namespace', {
      configurable: true,
      get: function() {
        var alias = app.alias || app._name
        if (app.parent) {
          return app.parent.namespace + '.' + alias;
        }
        return alias;
      }
    });
  };
};
