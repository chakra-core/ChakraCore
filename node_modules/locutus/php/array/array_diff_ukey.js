'use strict';

module.exports = function array_diff_ukey(arr1) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_diff_ukey/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: var $array1 = {blue: 1, red: 2, green: 3, purple: 4}
  //   example 1: var $array2 = {green: 5, blue: 6, yellow: 7, cyan: 8}
  //   example 1: array_diff_ukey($array1, $array2, function (key1, key2){ return (key1 === key2 ? 0 : (key1 > key2 ? 1 : -1)); })
  //   returns 1: {red: 2, purple: 4}

  var retArr = {};
  var arglm1 = arguments.length - 1;
  // var arglm2 = arglm1 - 1
  var cb = arguments[arglm1];
  var k1 = '';
  var i = 1;
  var arr = {};
  var k = '';

  var $global = typeof window !== 'undefined' ? window : global;

  cb = typeof cb === 'string' ? $global[cb] : Object.prototype.toString.call(cb) === '[object Array]' ? $global[cb[0]][cb[1]] : cb;

  arr1keys: for (k1 in arr1) {
    // eslint-disable-line no-labels
    for (i = 1; i < arglm1; i++) {
      arr = arguments[i];
      for (k in arr) {
        if (cb(k, k1) === 0) {
          // If it reaches here, it was found in at least one array, so try next value
          continue arr1keys; // eslint-disable-line no-labels
        }
      }
      retArr[k1] = arr1[k1];
    }
  }

  return retArr;
};
//# sourceMappingURL=array_diff_ukey.js.map