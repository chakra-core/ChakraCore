'use strict';

module.exports = function stristr(haystack, needle, bool) {
  //  discuss at: http://locutus.io/php/stristr/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  //   example 1: stristr('Kevin van Zonneveld', 'Van')
  //   returns 1: 'van Zonneveld'
  //   example 2: stristr('Kevin van Zonneveld', 'VAN', true)
  //   returns 2: 'Kevin '

  var pos = 0;

  haystack += '';
  pos = haystack.toLowerCase().indexOf((needle + '').toLowerCase());
  if (pos === -1) {
    return false;
  } else {
    if (bool) {
      return haystack.substr(0, pos);
    } else {
      return haystack.slice(pos);
    }
  }
};
//# sourceMappingURL=stristr.js.map