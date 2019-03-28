'use strict';

var isNumber = require('is-number');

/**
 * Expose `utils`
 */

var utils = module.exports;

utils.regex = {
 'w': /^(w((ee)?k)?s?)$/,
 'd': /^(d(ay)?s?)$/,
 'h': /^(h((ou)?r)?s?)$/,
 'm': /^(min(ute)?s?|m)$/,
 's': /^((sec(ond)?)s?|s)$/,
 'ms': /^(milli(second)?s?|ms)$/,
 'μs': /^(micro(second)?s?|μs)$/,
 'ns': /^(nano(second)?s?|ns?)$/,
};

utils.isSmallest = function(uom, unit) {
  return utils.regex[uom].test(unit);
};

utils.round = function(num, digits) {
  num = Math.abs(num);
  if (isNumber(digits)) {
    return num.toFixed(digits);
  }
  return Math.round(num);
};
