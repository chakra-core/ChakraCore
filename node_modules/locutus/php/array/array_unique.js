'use strict';

module.exports = function array_unique(inputArr) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_unique/
  // original by: Carlos R. L. Rodrigues (http://www.jsfromhell.com)
  //    input by: duncan
  //    input by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Nate
  // bugfixed by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  // improved by: Michael Grier
  //      note 1: The second argument, sort_flags is not implemented;
  //      note 1: also should be sorted (asort?) first according to docs
  //   example 1: array_unique(['Kevin','Kevin','van','Zonneveld','Kevin'])
  //   returns 1: {0: 'Kevin', 2: 'van', 3: 'Zonneveld'}
  //   example 2: array_unique({'a': 'green', 0: 'red', 'b': 'green', 1: 'blue', 2: 'red'})
  //   returns 2: {a: 'green', 0: 'red', 1: 'blue'}

  var key = '';
  var tmpArr2 = {};
  var val = '';

  var _arraySearch = function _arraySearch(needle, haystack) {
    var fkey = '';
    for (fkey in haystack) {
      if (haystack.hasOwnProperty(fkey)) {
        if (haystack[fkey] + '' === needle + '') {
          return fkey;
        }
      }
    }
    return false;
  };

  for (key in inputArr) {
    if (inputArr.hasOwnProperty(key)) {
      val = inputArr[key];
      if (_arraySearch(val, tmpArr2) === false) {
        tmpArr2[key] = val;
      }
    }
  }

  return tmpArr2;
};
//# sourceMappingURL=array_unique.js.map