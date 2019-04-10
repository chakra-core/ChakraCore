/*!
 * pretty-time <https://github.com/jonschlinkert/pretty-time>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isNumber = require('is-number');
var nano = require('nanoseconds');
var utils = require('./utils');
var scale = {
  'w': 6048e11,
  'd': 864e11,
  'h': 36e11,
  'm': 6e10,
  's': 1e9,
  'ms': 1e6,
  'μs': 1e3,
  'ns': 1,
};

function pretty(time, smallest, digits) {
  if (!isNumber(time) && !Array.isArray(time)) {
    throw new TypeError('expected an array or number in nanoseconds');
  }
  if (Array.isArray(time) && time.length !== 2) {
    throw new TypeError('expected an array from process.hrtime()');
  }

  if (isNumber(smallest)) {
    digits = smallest;
    smallest = null;
  }

  var num = isNumber(time) ? time : nano(time);
  var res = '';
  var prev;

  for (var uom in scale) {
    var step = scale[uom];
    var inc = num / step;

    if (smallest && utils.isSmallest(uom, smallest)) {
      inc = utils.round(inc, digits);
      if (prev && (inc === (prev / step))) --inc;
      res += inc + uom;
      return res.trim();
    }

    if (inc < 1) continue;
    if (!smallest) {
      inc = utils.round(inc, digits);
      res += inc + uom;
      return res;
    }

    prev = step;

    inc = Math.floor(inc);
    num -= (inc * step);
    res += inc + uom + ' ';
  }
  return res.trim();
}

/**
 * Expose `pretty`
 */

module.exports = pretty;
