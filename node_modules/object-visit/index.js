/*!
 * object-visit <https://github.com/jonschlinkert/object-visit>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isObject = require('isobject');

module.exports = function visit(thisArg, method, target) {
  if (!isObject(thisArg) && typeof thisArg !== 'function') {
    throw new Error('object-visit expects `thisArg` to be an object.');
  }

  if (typeof method !== 'string') {
    throw new Error('object-visit expects `method` name to be a string');
  }

  if (typeof thisArg[method] !== 'function') {
    return thisArg;
  }

  target = target || {};
  for (var key in target) {
    thisArg[method](key, target[key]);
  }
  return thisArg;
};
