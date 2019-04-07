'use strict';

module.exports = function strstr(haystack, needle, bool) {
  //  discuss at: http://locutus.io/php/strstr/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  // improved by: Kevin van Zonneveld (http://kvz.io)
  //   example 1: strstr('Kevin van Zonneveld', 'van')
  //   returns 1: 'van Zonneveld'
  //   example 2: strstr('Kevin van Zonneveld', 'van', true)
  //   returns 2: 'Kevin '
  //   example 3: strstr('name@example.com', '@')
  //   returns 3: '@example.com'
  //   example 4: strstr('name@example.com', '@', true)
  //   returns 4: 'name'

  var pos = 0;

  haystack += '';
  pos = haystack.indexOf(needle);
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
//# sourceMappingURL=strstr.js.map