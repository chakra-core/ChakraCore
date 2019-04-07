'use strict';

module.exports = function strchr(haystack, needle, bool) {
  //  discuss at: http://locutus.io/php/strchr/
  // original by: Philip Peterson
  //   example 1: strchr('Kevin van Zonneveld', 'van')
  //   returns 1: 'van Zonneveld'
  //   example 2: strchr('Kevin van Zonneveld', 'van', true)
  //   returns 2: 'Kevin '

  var strstr = require('../strings/strstr');
  return strstr(haystack, needle, bool);
};
//# sourceMappingURL=strchr.js.map