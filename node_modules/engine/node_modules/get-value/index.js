/*!
 * get-value <https://github.com/jonschlinkert/get-value>
 *
 * Copyright (c) 2014-2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var utils = require('./utils');

module.exports = function getValue(obj, prop, fn) {
  if (!utils.isObject(obj)) return {};
  if (Array.isArray(prop)) {
    prop = utils.flatten(prop).join('.');
  }

  if (typeof prop !== 'string') return obj;

  var path;

  if (fn && typeof fn === 'function') {
    path = fn(prop);
  } else if (fn === true) {
    path = escapePath(prop);
  } else {
    path = prop.split(/[[.\]]/).filter(Boolean);
  }

  var len = path.length, i = -1;
  var last = null;

  while(++i < len) {
    var key = path[i];
    last = obj[key];
    if (!last) { return last; }

    if (utils.isObject(obj)) {
      obj = last;
    }
  }
  return last;
};


function escape(str) {
  return str.split('\\.').join(utils.nonchars[0]);
}

function unescape(str) {
  return str.split(utils.nonchars[0]).join('.');
}

function escapePath(str) {
  return escape(str).split('.').map(function (seg) {
    return unescape(seg);
  });
}
