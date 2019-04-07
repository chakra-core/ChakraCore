'use strict';

module.exports = function hexdec(hexString) {
  //  discuss at: http://locutus.io/php/hexdec/
  // original by: Philippe Baumann
  //   example 1: hexdec('that')
  //   returns 1: 10
  //   example 2: hexdec('a0')
  //   returns 2: 160

  hexString = (hexString + '').replace(/[^a-f0-9]/gi, '');
  return parseInt(hexString, 16);
};
//# sourceMappingURL=hexdec.js.map