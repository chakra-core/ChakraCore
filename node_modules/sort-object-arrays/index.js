/*!
 * sort-object-arrays <https://github.com/jonschlinkert/sort-object-arrays>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var typeOf = require('kind-of');

/**
 * Recursively sort the array values in an object.
 */

function sortArrays(target, fn) {
  if (typeOf(target) !== 'object') {
    throw new TypeError('expected target to be an object');
  }

  for (var key in target) {
    var val = target[key];
    if (typeOf(val) === 'object') {
      target[key] = sortArrays(target[key], fn);
    } else if (Array.isArray(val)) {
      if (typeof val[0] === 'string') {
        val.sort(compare(fn));
      }
      target[key] = val;
    }
  }
  return target;
}

/**
 * Default comparison function to use for sorting
 */

function compare(fn) {
  if (typeof fn === 'function') {
    return fn;
  }
  return function(a, b) {
    return a.localeCompare(b);
  };
}

/**
 * Expose `sortArrays`
 */

module.exports = sortArrays;
