/*!
 * has-own-deep <https://github.com/jonschlinkert/has-own-deep>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var hasOwn = Object.prototype.hasOwnProperty;

module.exports = function hasOwnDeep(obj, key, escape) {
  if (typeof obj !== 'object') {
    throw new Error('has-own-deep expects an object');
  }

  if (typeof key !== 'string') {
    return false;
  }

  if (escape === true) {
    key = key.split('\\.').join('__TMP__');
  }

  var segs = key.split('.');

  if (escape === true) {
    var len = segs.length;
    while (len--) {
      segs[len] = segs[len].split('__TMP__').join('.');
    }
  }

  var len = segs.length, i = 0;
  while (len--) {
    var seg = segs[i++];
    if (!hasOwn.call(obj, seg)) {
      return false;
    }
    obj = obj[seg];
  }
  return true;
};
