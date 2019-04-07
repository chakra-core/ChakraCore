'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function is_finite(val) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/is_finite/
  // original by: Onno Marsman (https://twitter.com/onnomarsman)
  //   example 1: is_finite(Infinity)
  //   returns 1: false
  //   example 2: is_finite(-Infinity)
  //   returns 2: false
  //   example 3: is_finite(0)
  //   returns 3: true

  var warningType = '';

  if (val === Infinity || val === -Infinity) {
    return false;
  }

  // Some warnings for maximum PHP compatibility
  if ((typeof val === 'undefined' ? 'undefined' : _typeof(val)) === 'object') {
    warningType = Object.prototype.toString.call(val) === '[object Array]' ? 'array' : 'object';
  } else if (typeof val === 'string' && !val.match(/^[+-]?\d/)) {
    // simulate PHP's behaviour: '-9a' doesn't give a warning, but 'a9' does.
    warningType = 'string';
  }
  if (warningType) {
    var msg = 'Warning: is_finite() expects parameter 1 to be double, ' + warningType + ' given';
    throw new Error(msg);
  }

  return true;
};
//# sourceMappingURL=is_finite.js.map