/*!
 * omit-empty <https://github.com/jonschlinkert/omit-empty>
 *
 * Copyright (c) 2014-2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var typeOf = require('kind-of');
var hasValues = require('has-values');
var reduce = require('reduce-object');

function omitEmpty(o, options) {
  if (typeof options === 'boolean') {
    options = { noZero: options };
  }

  options = options || {};
  var excludeType = arrayify(options.excludeType);
  var exclude = arrayify(options.exclude);

  return reduce(o, function(acc, value, key) {
    if (exclude.indexOf(key) !== -1) {
      acc[key] = value;
      return acc;
    }

    if (excludeType.indexOf(typeOf(value)) !== -1) {
      acc[key] = value;
      return acc;
    }

    if (typeOf(value) === 'date') {
      acc[key] = value;
      return acc;
    }

    if (typeOf(value) === 'object') {
      var val = omitEmpty(value, options);
      if (hasValues(val)) {
        acc[key] = val;
      }
      return acc;
    }

    if (Array.isArray(value)) {
      value = emptyArray(value, options);
    }

    if (typeof value === 'function' || hasValues(value, options.noZero)) {
      acc[key] = value;
    }
    return acc;
  }, {});
};

/**
 * Omit empty array values
 */

function emptyArray(arr, options) {
  var len = arr.length;
  var idx = -1;
  var res = [];
  while (++idx < len) {
    var ele = arr[idx];
    if (typeof ele === 'undefined' || ele === '' || ele === null) {
      continue;
    }
    if (typeOf(ele) === 'date') {
      res.push(ele);
    }
    if (typeOf(ele) === 'object') {
      ele = omitEmpty(ele, options);
    }
    if (Array.isArray(ele)) {
      ele = emptyArray(ele, options);
    }
    if (typeof ele === 'function' || hasValues(ele, options.noZero)) {
      res.push(ele);
    }
  }
  return res;
}

/**
 * Cast `val` to an array
 */

function arrayify(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
}

/**
 * Expose `omitEmpty`
 */

module.exports = omitEmpty;
