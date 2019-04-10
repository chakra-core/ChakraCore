'use strict';

module.exports = function strncasecmp(argStr1, argStr2, len) {
  //  discuss at: http://locutus.io/php/strncasecmp/
  // original by: Saulo Vallory
  //    input by: Nate
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  //      note 1: Returns < 0 if str1 is less than str2 ; > 0
  //      note 1: if str1 is greater than str2, and 0 if they are equal.
  //   example 1: strncasecmp('Price 12.9', 'Price 12.15', 2)
  //   returns 1: 0
  //   example 2: strncasecmp('Price 12.09', 'Price 12.15', 10)
  //   returns 2: -1
  //   example 3: strncasecmp('Price 12.90', 'Price 12.15', 30)
  //   returns 3: 8
  //   example 4: strncasecmp('Version 12.9', 'Version 12.15', 20)
  //   returns 4: 8
  //   example 5: strncasecmp('Version 12.15', 'Version 12.9', 20)
  //   returns 5: -8

  var diff;
  var i = 0;
  var str1 = (argStr1 + '').toLowerCase().substr(0, len);
  var str2 = (argStr2 + '').toLowerCase().substr(0, len);

  if (str1.length !== str2.length) {
    if (str1.length < str2.length) {
      len = str1.length;
      if (str2.substr(0, str1.length) === str1) {
        // return the difference of chars
        return str1.length - str2.length;
      }
    } else {
      len = str2.length;
      // str1 is longer than str2
      if (str1.substr(0, str2.length) === str2) {
        // return the difference of chars
        return str1.length - str2.length;
      }
    }
  } else {
    // Avoids trying to get a char that does not exist
    len = str1.length;
  }

  for (diff = 0, i = 0; i < len; i++) {
    diff = str1.charCodeAt(i) - str2.charCodeAt(i);
    if (diff !== 0) {
      return diff;
    }
  }

  return 0;
};
//# sourceMappingURL=strncasecmp.js.map