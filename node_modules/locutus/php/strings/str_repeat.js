'use strict';

module.exports = function str_repeat(input, multiplier) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/str_repeat/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
  // improved by: Ian Carter (http://euona.com/)
  //   example 1: str_repeat('-=', 10)
  //   returns 1: '-=-=-=-=-=-=-=-=-=-='

  var y = '';
  while (true) {
    if (multiplier & 1) {
      y += input;
    }
    multiplier >>= 1;
    if (multiplier) {
      input += input;
    } else {
      break;
    }
  }
  return y;
};
//# sourceMappingURL=str_repeat.js.map