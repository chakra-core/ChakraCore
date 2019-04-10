"use strict";

module.exports = function array_shift(inputArr) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_shift/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Martijn Wieringa
  //      note 1: Currently does not handle objects
  //   example 1: array_shift(['Kevin', 'van', 'Zonneveld'])
  //   returns 1: 'Kevin'

  var _checkToUpIndices = function _checkToUpIndices(arr, ct, key) {
    // Deal with situation, e.g., if encounter index 4 and try
    // to set it to 0, but 0 exists later in loop (need to
    // increment all subsequent (skipping current key, since
    // we need its value below) until find unused)
    if (arr[ct] !== undefined) {
      var tmp = ct;
      ct += 1;
      if (ct === key) {
        ct += 1;
      }
      ct = _checkToUpIndices(arr, ct, key);
      arr[ct] = arr[tmp];
      delete arr[tmp];
    }

    return ct;
  };

  if (inputArr.length === 0) {
    return null;
  }
  if (inputArr.length > 0) {
    return inputArr.shift();
  }
};
//# sourceMappingURL=array_shift.js.map