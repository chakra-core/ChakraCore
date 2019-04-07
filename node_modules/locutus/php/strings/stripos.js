'use strict';

module.exports = function stripos(fHaystack, fNeedle, fOffset) {
  //  discuss at: http://locutus.io/php/stripos/
  // original by: Martijn Wieringa
  //  revised by: Onno Marsman (https://twitter.com/onnomarsman)
  //   example 1: stripos('ABC', 'a')
  //   returns 1: 0

  var haystack = (fHaystack + '').toLowerCase();
  var needle = (fNeedle + '').toLowerCase();
  var index = 0;

  if ((index = haystack.indexOf(needle, fOffset)) !== -1) {
    return index;
  }

  return false;
};
//# sourceMappingURL=stripos.js.map