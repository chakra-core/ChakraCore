"use strict";

module.exports = function array_fill(startIndex, num, mixedVal) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_fill/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Waldo Malqui Silva (http://waldo.malqui.info)
  //   example 1: array_fill(5, 6, 'banana')
  //   returns 1: { 5: 'banana', 6: 'banana', 7: 'banana', 8: 'banana', 9: 'banana', 10: 'banana' }

  var key;
  var tmpArr = {};

  if (!isNaN(startIndex) && !isNaN(num)) {
    for (key = 0; key < num; key++) {
      tmpArr[key + startIndex] = mixedVal;
    }
  }

  return tmpArr;
};
//# sourceMappingURL=array_fill.js.map