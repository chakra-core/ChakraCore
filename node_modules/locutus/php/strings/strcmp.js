"use strict";

module.exports = function strcmp(str1, str2) {
  //  discuss at: http://locutus.io/php/strcmp/
  // original by: Waldo Malqui Silva (http://waldo.malqui.info)
  //    input by: Steve Hilder
  // improved by: Kevin van Zonneveld (http://kvz.io)
  //  revised by: gorthaur
  //   example 1: strcmp( 'waldo', 'owald' )
  //   returns 1: 1
  //   example 2: strcmp( 'owald', 'waldo' )
  //   returns 2: -1

  return str1 === str2 ? 0 : str1 > str2 ? 1 : -1;
};
//# sourceMappingURL=strcmp.js.map