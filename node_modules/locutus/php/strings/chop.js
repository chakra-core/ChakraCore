'use strict';

module.exports = function chop(str, charlist) {
  //  discuss at: http://locutus.io/php/chop/
  // original by: Paulo Freitas
  //   example 1: chop('    Kevin van Zonneveld    ')
  //   returns 1: '    Kevin van Zonneveld'

  var rtrim = require('../strings/rtrim');
  return rtrim(str, charlist);
};
//# sourceMappingURL=chop.js.map