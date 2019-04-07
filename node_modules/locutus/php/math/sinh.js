"use strict";

module.exports = function sinh(arg) {
  //  discuss at: http://locutus.io/php/sinh/
  // original by: Onno Marsman (https://twitter.com/onnomarsman)
  //   example 1: sinh(-0.9834330348825909)
  //   returns 1: -1.1497971402636502

  return (Math.exp(arg) - Math.exp(-arg)) / 2;
};
//# sourceMappingURL=sinh.js.map