"use strict";

module.exports = function decbin(number) {
  //  discuss at: http://locutus.io/php/decbin/
  // original by: Enrique Gonzalez
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  // improved by: http://stackoverflow.com/questions/57803/how-to-convert-decimal-to-hex-in-javascript
  //    input by: pilus
  //    input by: nord_ua
  //   example 1: decbin(12)
  //   returns 1: '1100'
  //   example 2: decbin(26)
  //   returns 2: '11010'
  //   example 3: decbin('26')
  //   returns 3: '11010'

  if (number < 0) {
    number = 0xFFFFFFFF + number + 1;
  }
  return parseInt(number, 10).toString(2);
};
//# sourceMappingURL=decbin.js.map