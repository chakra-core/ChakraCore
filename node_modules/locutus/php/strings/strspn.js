"use strict";

module.exports = function strspn(str1, str2, start, lgth) {
  //  discuss at: http://locutus.io/php/strspn/
  // original by: Valentina De Rosa
  // improved by: Brett Zamir (http://brett-zamir.me)
  //   example 1: strspn('42 is the answer, what is the question ...', '1234567890')
  //   returns 1: 2
  //   example 2: strspn('foo', 'o', 1, 2)
  //   returns 2: 2

  var found;
  var stri;
  var strj;
  var j = 0;
  var i = 0;

  start = start ? start < 0 ? str1.length + start : start : 0;
  lgth = lgth ? lgth < 0 ? str1.length + lgth - start : lgth : str1.length - start;
  str1 = str1.substr(start, lgth);

  for (i = 0; i < str1.length; i++) {
    found = 0;
    stri = str1.substring(i, i + 1);
    for (j = 0; j <= str2.length; j++) {
      strj = str2.substring(j, j + 1);
      if (stri === strj) {
        found = 1;
        break;
      }
    }
    if (found !== 1) {
      return i;
    }
  }

  return i;
};
//# sourceMappingURL=strspn.js.map