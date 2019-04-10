/*!
 * template-helpers <https://github.com/jonschlinkert/template-helpers>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

/**
 * Expose helpers
 */

module.exports = function(key) {
  var lib = require('./lib');
  var forIn = lib.object.forIn;
  var helpers = {};

  if (typeof key === 'string') {
    helpers = lib[key];
    helpers[key] = helpers;
    return helpers;
  }

  if (Array.isArray(key)) {
    return key.reduce(function(acc, k) {
      acc[k] = lib[k];

      forIn(acc[k], function(group, prop) {
        acc[prop] = group;
      });

      return acc;
    }, {});
  }

  forIn(lib, function(group, prop) {
    helpers[prop] = group;

    forIn(group, function(v, k) {
      helpers[k] = v;
    });
  });

  return helpers;
};
