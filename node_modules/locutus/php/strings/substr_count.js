'use strict';

module.exports = function substr_count(haystack, needle, offset, length) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/substr_count/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  // improved by: Brett Zamir (http://brett-zamir.me)
  // improved by: Thomas
  //   example 1: substr_count('Kevin van Zonneveld', 'e')
  //   returns 1: 3
  //   example 2: substr_count('Kevin van Zonneveld', 'K', 1)
  //   returns 2: 0
  //   example 3: substr_count('Kevin van Zonneveld', 'Z', 0, 10)
  //   returns 3: false

  var cnt = 0;

  haystack += '';
  needle += '';
  if (isNaN(offset)) {
    offset = 0;
  }
  if (isNaN(length)) {
    length = 0;
  }
  if (needle.length === 0) {
    return false;
  }
  offset--;

  while ((offset = haystack.indexOf(needle, offset + 1)) !== -1) {
    if (length > 0 && offset + needle.length > length) {
      return false;
    }
    cnt++;
  }

  return cnt;
};
//# sourceMappingURL=substr_count.js.map