/*!
 * omit-empty <https://github.com/jonschlinkert/omit-empty>
 *
 * Copyright (c) 2014-2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isDateObject = require('is-date-object');
var reduce = require('reduce-object');
var hasValues = require('has-values');
var isObject = require('isobject');

function omitEmpty(o, noZero) {
  return reduce(o, function(acc, value, key) {
    if (isDateObject(value)) {
      acc[key] = value;
      return acc;
    }

    if (isObject(value)) {
      var val = omitEmpty(value, noZero);
      if (hasValues(val)) {
        acc[key] = val;
      }
      return acc;
    }

    if (Array.isArray(value)) {
      value = emptyArray(value);
    }

    if (typeof value === 'function' || hasValues(value, noZero)) {
      acc[key] = value;
    }
    return acc;
  }, {});
};

function emptyArray(arr) {
  var len = arr.length;
  var idx = -1;
  var res = [];
  while (++idx < len) {
    var ele = arr[idx];
    if (typeof ele === 'undefined' || ele === '' || ele === null) {
      continue;
    }
    res.push(ele);
  }
  return res;
}

/**
 * Expose `omitEmpty`
 */

module.exports = omitEmpty;
