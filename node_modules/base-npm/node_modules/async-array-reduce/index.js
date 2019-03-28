/*!
 * async-array-reduce <https://github.com/jonschlinkert/async-array-reduce>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

module.exports = function reduce(arr, init, fn, cb) {
  if (typeof cb !== 'function') {
    throw new TypeError('a callback function is expected');
  }

  (function next(i, acc) {
    if (i === arr.length) {
      return cb(null, acc);
    }
    fn(acc, arr[i], function(err, val) {
      if (err) return cb(err);
      next(i + 1, val);
    });
  })(0, init);
};
