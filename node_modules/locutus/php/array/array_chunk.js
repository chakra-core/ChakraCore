'use strict';

module.exports = function array_chunk(input, size, preserveKeys) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_chunk/
  // original by: Carlos R. L. Rodrigues (http://www.jsfromhell.com)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //      note 1: Important note: Per the ECMAScript specification,
  //      note 1: objects may not always iterate in a predictable order
  //   example 1: array_chunk(['Kevin', 'van', 'Zonneveld'], 2)
  //   returns 1: [['Kevin', 'van'], ['Zonneveld']]
  //   example 2: array_chunk(['Kevin', 'van', 'Zonneveld'], 2, true)
  //   returns 2: [{0:'Kevin', 1:'van'}, {2: 'Zonneveld'}]
  //   example 3: array_chunk({1:'Kevin', 2:'van', 3:'Zonneveld'}, 2)
  //   returns 3: [['Kevin', 'van'], ['Zonneveld']]
  //   example 4: array_chunk({1:'Kevin', 2:'van', 3:'Zonneveld'}, 2, true)
  //   returns 4: [{1: 'Kevin', 2: 'van'}, {3: 'Zonneveld'}]

  var x;
  var p = '';
  var i = 0;
  var c = -1;
  var l = input.length || 0;
  var n = [];

  if (size < 1) {
    return null;
  }

  if (Object.prototype.toString.call(input) === '[object Array]') {
    if (preserveKeys) {
      while (i < l) {
        (x = i % size) ? n[c][i] = input[i] : n[++c] = {};n[c][i] = input[i];
        i++;
      }
    } else {
      while (i < l) {
        (x = i % size) ? n[c][x] = input[i] : n[++c] = [input[i]];
        i++;
      }
    }
  } else {
    if (preserveKeys) {
      for (p in input) {
        if (input.hasOwnProperty(p)) {
          (x = i % size) ? n[c][p] = input[p] : n[++c] = {};n[c][p] = input[p];
          i++;
        }
      }
    } else {
      for (p in input) {
        if (input.hasOwnProperty(p)) {
          (x = i % size) ? n[c][x] = input[p] : n[++c] = [input[p]];
          i++;
        }
      }
    }
  }

  return n;
};
//# sourceMappingURL=array_chunk.js.map