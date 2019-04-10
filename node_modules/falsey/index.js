/*!
 * falsey <https://github.com/jonschlinkert/falsey>
 *
 * Copyright (c) 2014-2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var typeOf = require('kind-of');

function falsey(val, keywords) {
  if (!val) return true;

  if (Array.isArray(val) || typeOf(val) === 'arguments') {
    return !val.length;
  }

  if (typeOf(val) === 'object') {
    return !Object.keys(val).length;
  }

  var arr = !keywords
    ? falsey.keywords
    : arrayify(keywords);

  return arr.indexOf(val.toLowerCase ? val.toLowerCase() : val) !== -1;
}

/**
 * Expose `keywords`
 */

falsey.keywords = ['none', 'nil', 'nope', 'no', 'nada', '0', 'false'];

function arrayify(val) {
  return Array.isArray(val) ? val : [val];
}

module.exports = falsey;
