'use strict';

module.exports = function array_udiff(arr1) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_udiff/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: var $array1 = {a: 'green', b: 'brown', c: 'blue', 0: 'red'}
  //   example 1: var $array2 = {a: 'GREEN', B: 'brown', 0: 'yellow', 1: 'red'}
  //   example 1: array_udiff($array1, $array2, function (f_string1, f_string2){var string1 = (f_string1+'').toLowerCase(); var string2 = (f_string2+'').toLowerCase(); if (string1 > string2) return 1; if (string1 === string2) return 0; return -1;})
  //   returns 1: {c: 'blue'}

  var retArr = {};
  var arglm1 = arguments.length - 1;
  var cb = arguments[arglm1];
  var arr = '';
  var i = 1;
  var k1 = '';
  var k = '';

  var $global = typeof window !== 'undefined' ? window : global;

  cb = typeof cb === 'string' ? $global[cb] : Object.prototype.toString.call(cb) === '[object Array]' ? $global[cb[0]][cb[1]] : cb;

  arr1keys: for (k1 in arr1) {
    // eslint-disable-line no-labels
    for (i = 1; i < arglm1; i++) {
      // eslint-disable-line no-labels
      arr = arguments[i];
      for (k in arr) {
        if (cb(arr[k], arr1[k1]) === 0) {
          // If it reaches here, it was found in at least one array, so try next value
          continue arr1keys; // eslint-disable-line no-labels
        }
      }
      retArr[k1] = arr1[k1];
    }
  }

  return retArr;
};
//# sourceMappingURL=array_udiff.js.map