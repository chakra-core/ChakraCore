"use strict";

module.exports = function array_reduce(aInput, callback) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_reduce/
  // original by: Alfonso Jimenez (http://www.alfonsojimenez.com)
  //      note 1: Takes a function as an argument, not a function's name
  //   example 1: array_reduce([1, 2, 3, 4, 5], function (v, w){v += w;return v;})
  //   returns 1: 15

  var lon = aInput.length;
  var res = 0;
  var i = 0;
  var tmp = [];

  for (i = 0; i < lon; i += 2) {
    tmp[0] = aInput[i];
    if (aInput[i + 1]) {
      tmp[1] = aInput[i + 1];
    } else {
      tmp[1] = 0;
    }
    res += callback.apply(null, tmp);
    tmp = [];
  }

  return res;
};
//# sourceMappingURL=array_reduce.js.map