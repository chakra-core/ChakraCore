"use strict";

module.exports = function dechex(number) {
  //  discuss at: http://locutus.io/php/dechex/
  // original by: Philippe Baumann
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  // improved by: http://stackoverflow.com/questions/57803/how-to-convert-decimal-to-hex-in-javascript
  //    input by: pilus
  //   example 1: dechex(10)
  //   returns 1: 'a'
  //   example 2: dechex(47)
  //   returns 2: '2f'
  //   example 3: dechex(-1415723993)
  //   returns 3: 'ab9dc427'

  if (number < 0) {
    number = 0xFFFFFFFF + number + 1;
  }
  return parseInt(number, 10).toString(16);
};
//# sourceMappingURL=dechex.js.map