'use strict';

module.exports = function array_pad(input, padSize, padValue) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_pad/
  // original by: Waldo Malqui Silva (http://waldo.malqui.info)
  //   example 1: array_pad([ 7, 8, 9 ], 2, 'a')
  //   returns 1: [ 7, 8, 9]
  //   example 2: array_pad([ 7, 8, 9 ], 5, 'a')
  //   returns 2: [ 7, 8, 9, 'a', 'a']
  //   example 3: array_pad([ 7, 8, 9 ], 5, 2)
  //   returns 3: [ 7, 8, 9, 2, 2]
  //   example 4: array_pad([ 7, 8, 9 ], -5, 'a')
  //   returns 4: [ 'a', 'a', 7, 8, 9 ]

  var pad = [];
  var newArray = [];
  var newLength;
  var diff = 0;
  var i = 0;

  if (Object.prototype.toString.call(input) === '[object Array]' && !isNaN(padSize)) {
    newLength = padSize < 0 ? padSize * -1 : padSize;
    diff = newLength - input.length;

    if (diff > 0) {
      for (i = 0; i < diff; i++) {
        newArray[i] = padValue;
      }
      pad = padSize < 0 ? newArray.concat(input) : input.concat(newArray);
    } else {
      pad = input;
    }
  }

  return pad;
};
//# sourceMappingURL=array_pad.js.map