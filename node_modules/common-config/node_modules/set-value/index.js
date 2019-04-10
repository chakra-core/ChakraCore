/*!
 * set-value <https://github.com/jonschlinkert/set-value>
 *
 * Copyright (c) 2014-2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var toPath = require('to-object-path');
var extend = require('extend-shallow');
var isObject = require('isobject');

module.exports = function(obj, path, val) {
  if (typeof obj !== 'object') {
    return obj;
  }

  if (Array.isArray(path)) {
    path = toPath(path);
  }

  if (typeof path !== 'string') {
    return obj;
  }

  var segs = path.split('.');
  var len = segs.length, i = -1;
  var res = obj;
  var last;

  while (++i < len) {
    var key = segs[i];

    while (key[key.length - 1] === '\\') {
      key = key.slice(0, -1) + '.' + segs[++i];
    }

    if (i === len - 1) {
      last = key;
      break;
    }

    if (typeof obj[key] !== 'object') {
      obj[key] = {};
    }
    obj = obj[key];
  }

  if (obj.hasOwnProperty(last) && typeof obj[last] === 'object') {
    if (isObject(val)) {
      extend(obj[last], val);
    } else {
      obj[last] = val;
    }

  } else {
    obj[last] = val;
  }
  return res;
};
