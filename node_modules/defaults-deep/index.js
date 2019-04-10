/*!
 * defaults-deep <https://github.com/jonschlinkert/defaults-deep>
 *
 * Copyright (c) 2014-2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var lazy = require('lazy-cache')(require);
lazy('is-extendable', 'isObject');
lazy('for-own', 'forOwn');

function defaultsDeep(target, objects) {
  target = target || {};

  function copy(target, current) {
    lazy.forOwn(current, function (value, key) {
      if (key === '__proto__') {
        return;
      }

      var val = target[key];
      // add the missing property, or allow a null property to be updated
      if (val == null) {
        target[key] = value;
      } else if (lazy.isObject(val) && lazy.isObject(value)) {
        defaultsDeep(val, value);
      }
    });
  }

  var len = arguments.length, i = 0;
  while (i < len) {
    var obj = arguments[i++];
    if (obj) {
      copy(target, obj);
    }
  }
  return target;
};

/**
 * Expose `defaultsDeep`
 */

module.exports = defaultsDeep;
