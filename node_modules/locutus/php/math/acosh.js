"use strict";

module.exports = function acosh(arg) {
  //  discuss at: http://locutus.io/php/acosh/
  // original by: Onno Marsman (https://twitter.com/onnomarsman)
  //   example 1: acosh(8723321.4)
  //   returns 1: 16.674657798418625

  return Math.log(arg + Math.sqrt(arg * arg - 1));
};
//# sourceMappingURL=acosh.js.map