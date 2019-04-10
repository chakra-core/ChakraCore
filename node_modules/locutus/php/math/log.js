'use strict';

module.exports = function log(arg, base) {
  //  discuss at: http://locutus.io/php/log/
  // original by: Onno Marsman (https://twitter.com/onnomarsman)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //   example 1: log(8723321.4, 7)
  //   returns 1: 8.212871815082147

  return typeof base === 'undefined' ? Math.log(arg) : Math.log(arg) / Math.log(base);
};
//# sourceMappingURL=log.js.map