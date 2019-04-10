'use strict';

module.exports = function strcspn(str, mask, start, length) {
  //  discuss at: http://locutus.io/php/strcspn/
  // original by: Brett Zamir (http://brett-zamir.me)
  //  revised by: Theriault
  //   example 1: strcspn('abcdefg123', '1234567890')
  //   returns 1: 7
  //   example 2: strcspn('123abc', '1234567890')
  //   returns 2: 0
  //   example 3: strcspn('abcdefg123', '1234567890', 1)
  //   returns 3: 6
  //   example 4: strcspn('abcdefg123', '1234567890', -6, -5)
  //   returns 4: 1

  start = start || 0;
  length = typeof length === 'undefined' ? str.length : length || 0;
  if (start < 0) start = str.length + start;
  if (length < 0) length = str.length - start + length;
  if (start < 0 || start >= str.length || length <= 0 || e >= str.length) return 0;
  var e = Math.min(str.length, start + length);
  for (var i = start, lgth = 0; i < e; i++) {
    if (mask.indexOf(str.charAt(i)) !== -1) {
      break;
    }
    ++lgth;
  }
  return lgth;
};
//# sourceMappingURL=strcspn.js.map