'use strict';

module.exports = function bindec(binaryString) {
  //  discuss at: http://locutus.io/php/bindec/
  // original by: Philippe Baumann
  //   example 1: bindec('110011')
  //   returns 1: 51
  //   example 2: bindec('000110011')
  //   returns 2: 51
  //   example 3: bindec('111')
  //   returns 3: 7

  binaryString = (binaryString + '').replace(/[^01]/gi, '');

  return parseInt(binaryString, 2);
};
//# sourceMappingURL=bindec.js.map