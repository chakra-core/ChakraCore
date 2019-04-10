/*!
 * async-array-reduce <https://github.com/jonschlinkert/async-array-reduce>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

module.exports = function reduce(arr, memo, iteratee, cb) {
  if (!Array.isArray(arr)) {
    throw new TypeError('expected an array');
  }
  if (typeof iteratee !== 'function') {
    throw new TypeError('expected iteratee to be a function');
  }
  if (typeof cb !== 'function') {
    throw new TypeError('expected callback to be a function');
  }

  var callback = once(cb);

  (function next(i, acc) {
    if (i === arr.length) {
      callback(null, acc);
      return;
    }

    iteratee(acc, arr[i], function(err, val) {
      if (err) {
        callback(err);
        return;
      }
      next(i + 1, val);
    });
  })(0, memo);
};

function once(fn) {
  var value;
  return function() {
    if (!fn.called) {
      fn.called = true;
      value = fn.apply(this, arguments);
    }
    return value;
  };
}
