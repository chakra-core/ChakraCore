'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function array_change_key_case(array, cs) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_change_key_case/
  // original by: Ates Goral (http://magnetiq.com)
  // improved by: marrtins
  // improved by: Brett Zamir (http://brett-zamir.me)
  //   example 1: array_change_key_case(42)
  //   returns 1: false
  //   example 2: array_change_key_case([ 3, 5 ])
  //   returns 2: [3, 5]
  //   example 3: array_change_key_case({ FuBaR: 42 })
  //   returns 3: {"fubar": 42}
  //   example 4: array_change_key_case({ FuBaR: 42 }, 'CASE_LOWER')
  //   returns 4: {"fubar": 42}
  //   example 5: array_change_key_case({ FuBaR: 42 }, 'CASE_UPPER')
  //   returns 5: {"FUBAR": 42}
  //   example 6: array_change_key_case({ FuBaR: 42 }, 2)
  //   returns 6: {"FUBAR": 42}

  var caseFnc;
  var key;
  var tmpArr = {};

  if (Object.prototype.toString.call(array) === '[object Array]') {
    return array;
  }

  if (array && (typeof array === 'undefined' ? 'undefined' : _typeof(array)) === 'object') {
    caseFnc = !cs || cs === 'CASE_LOWER' ? 'toLowerCase' : 'toUpperCase';
    for (key in array) {
      tmpArr[key[caseFnc]()] = array[key];
    }
    return tmpArr;
  }

  return false;
};
//# sourceMappingURL=array_change_key_case.js.map