'use strict';

module.exports = function hex2bin(s) {
  //  discuss at: http://locutus.io/php/hex2bin/
  // original by: Dumitru Uzun (http://duzun.me)
  //   example 1: hex2bin('44696d61')
  //   returns 1: 'Dima'
  //   example 2: hex2bin('00')
  //   returns 2: '\x00'
  //   example 3: hex2bin('2f1q')
  //   returns 3: false

  var ret = [];
  var i = 0;
  var l;

  s += '';

  for (l = s.length; i < l; i += 2) {
    var c = parseInt(s.substr(i, 1), 16);
    var k = parseInt(s.substr(i + 1, 1), 16);
    if (isNaN(c) || isNaN(k)) return false;
    ret.push(c << 4 | k);
  }

  return String.fromCharCode.apply(String, ret);
};
//# sourceMappingURL=hex2bin.js.map