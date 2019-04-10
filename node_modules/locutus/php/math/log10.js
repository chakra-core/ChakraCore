"use strict";

module.exports = function log10(arg) {
  //  discuss at: http://locutus.io/php/log10/
  // original by: Philip Peterson
  // improved by: Onno Marsman (https://twitter.com/onnomarsman)
  // improved by: Tod Gentille
  // improved by: Brett Zamir (http://brett-zamir.me)
  //   example 1: log10(10)
  //   returns 1: 1
  //   example 2: log10(1)
  //   returns 2: 0

  return Math.log(arg) / 2.302585092994046; // Math.LN10
};
//# sourceMappingURL=log10.js.map