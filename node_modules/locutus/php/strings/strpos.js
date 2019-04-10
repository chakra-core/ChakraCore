'use strict';

module.exports = function strpos(haystack, needle, offset) {
  //  discuss at: http://locutus.io/php/strpos/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Onno Marsman (https://twitter.com/onnomarsman)
  // improved by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: Daniel Esteban
  //   example 1: strpos('Kevin van Zonneveld', 'e', 5)
  //   returns 1: 14

  var i = (haystack + '').indexOf(needle, offset || 0);
  return i === -1 ? false : i;
};
//# sourceMappingURL=strpos.js.map