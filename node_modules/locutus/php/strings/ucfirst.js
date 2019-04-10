'use strict';

module.exports = function ucfirst(str) {
  //  discuss at: http://locutus.io/php/ucfirst/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //   example 1: ucfirst('kevin van zonneveld')
  //   returns 1: 'Kevin van zonneveld'

  str += '';
  var f = str.charAt(0).toUpperCase();
  return f + str.substr(1);
};
//# sourceMappingURL=ucfirst.js.map