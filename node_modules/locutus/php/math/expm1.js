"use strict";

module.exports = function expm1(x) {
  //  discuss at: http://locutus.io/php/expm1/
  // original by: Brett Zamir (http://brett-zamir.me)
  // improved by: Robert Eisele (http://www.xarg.org/)
  //      note 1: Precision 'n' can be adjusted as desired
  //   example 1: expm1(1e-15)
  //   returns 1: 1.0000000000000007e-15

  return x < 1e-5 && x > -1e-5 ? x + 0.5 * x * x : Math.exp(x) - 1;
};
//# sourceMappingURL=expm1.js.map