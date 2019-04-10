"use strict";

module.exports = function range(low, high, step) {
  //  discuss at: http://locutus.io/php/range/
  // original by: Waldo Malqui Silva (http://waldo.malqui.info)
  //   example 1: range ( 0, 12 )
  //   returns 1: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
  //   example 2: range( 0, 100, 10 )
  //   returns 2: [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
  //   example 3: range( 'a', 'i' )
  //   returns 3: ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i']
  //   example 4: range( 'c', 'a' )
  //   returns 4: ['c', 'b', 'a']

  var matrix = [];
  var iVal;
  var endval;
  var plus;
  var walker = step || 1;
  var chars = false;

  if (!isNaN(low) && !isNaN(high)) {
    iVal = low;
    endval = high;
  } else if (isNaN(low) && isNaN(high)) {
    chars = true;
    iVal = low.charCodeAt(0);
    endval = high.charCodeAt(0);
  } else {
    iVal = isNaN(low) ? 0 : low;
    endval = isNaN(high) ? 0 : high;
  }

  plus = !(iVal > endval);
  if (plus) {
    while (iVal <= endval) {
      matrix.push(chars ? String.fromCharCode(iVal) : iVal);
      iVal += walker;
    }
  } else {
    while (iVal >= endval) {
      matrix.push(chars ? String.fromCharCode(iVal) : iVal);
      iVal -= walker;
    }
  }

  return matrix;
};
//# sourceMappingURL=range.js.map