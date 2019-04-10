/*!
 * is-valid-instance (https://github.com/jonschlinkert/is-valid-instance)
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isObject = require('isobject');
var pascal = require('pascalcase');

/**
 * By default a valid instance is an instance of `Base` with an `isApp` property set to `true`.
 *
 * ```js
 * function plugin(app) {
 *   if (!isValidInstance(app)) return;
 *   // do plugin stuff
 * }
 * ```
 *
 * @param {Object} `val`
 * @param {Array|Function} `names` One or more names to check for on the given instance. Example `app` will check for `app.isApp === true` or `app._name === 'app'`.
 * @param {Function} `fn` Custom function for validating the instance.
 * @return {Boolean}
 * @api public
 */

module.exports = function(val, names, fn) {
  if (!isObject(val)) {
    return false;
  }

  // if it's not an instance of `Base`, and it's not a vinyl file, return false
  if (!val.isBase && !val._isVinyl) {
    return false;
  }

  if (arrayify(names).indexOf('*') === 0 || arrayify(names).indexOf('any') === 0) {
    return true;
  }

  if (typeof names === 'function') {
    fn = names;
    names = undefined;
  }

  if (typeof fn === 'function') {
    return fn(val, names);
  }

  if (typeof names === 'undefined' && val.isApp !== true) {
    return false;
  }

  var ctor = val.constructor;
  return hasAnyType(val, names, val._name) === true
    || hasAnyType(val, names, ctor.name) === true
    || hasAnyType(val, names, toName(val._name)) === true;
};

function hasAnyType(val, names, name) {
  if (typeof names === 'undefined') {
    return true;
  }

  if (!name) {
    return false;
  }

  name = name.toLowerCase();
  var types = ['file', 'vinyl', 'view'];
  var arr = arrayify(names);
  var len = arr.length;
  var idx = -1;

  while (++idx < len) {
    var type = arr[idx];
    if (isType(val, type, name)) {
      return true;
    }
    if (types.indexOf(type) !== -1 && types.indexOf(name) !== -1) {
      return true;
    }
  }
  return false;
}

function isType(val, type, name) {
  return val[isName(type)] === true || val._name === type || name === type;
}

function isName(type) {
  if (!type) return;
  return 'is' + pascal(type.toString());
}

function toName(str) {
  if (!str) return;
  return str.replace(/^is(?=.)/, '').toLowerCase();
}

function arrayify(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
}
