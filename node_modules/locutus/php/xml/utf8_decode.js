'use strict';

module.exports = function utf8_decode(strData) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/utf8_decode/
  // original by: Webtoolkit.info (http://www.webtoolkit.info/)
  //    input by: Aman Gupta
  //    input by: Brett Zamir (http://brett-zamir.me)
  // improved by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Norman "zEh" Fuchs
  // bugfixed by: hitwork
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  // bugfixed by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: kirilloid
  // bugfixed by: w35l3y (http://www.wesley.eti.br)
  //   example 1: utf8_decode('Kevin van Zonneveld')
  //   returns 1: 'Kevin van Zonneveld'

  var tmpArr = [];
  var i = 0;
  var c1 = 0;
  var seqlen = 0;

  strData += '';

  while (i < strData.length) {
    c1 = strData.charCodeAt(i) & 0xFF;
    seqlen = 0;

    // http://en.wikipedia.org/wiki/UTF-8#Codepage_layout
    if (c1 <= 0xBF) {
      c1 = c1 & 0x7F;
      seqlen = 1;
    } else if (c1 <= 0xDF) {
      c1 = c1 & 0x1F;
      seqlen = 2;
    } else if (c1 <= 0xEF) {
      c1 = c1 & 0x0F;
      seqlen = 3;
    } else {
      c1 = c1 & 0x07;
      seqlen = 4;
    }

    for (var ai = 1; ai < seqlen; ++ai) {
      c1 = c1 << 0x06 | strData.charCodeAt(ai + i) & 0x3F;
    }

    if (seqlen === 4) {
      c1 -= 0x10000;
      tmpArr.push(String.fromCharCode(0xD800 | c1 >> 10 & 0x3FF));
      tmpArr.push(String.fromCharCode(0xDC00 | c1 & 0x3FF));
    } else {
      tmpArr.push(String.fromCharCode(c1));
    }

    i += seqlen;
  }

  return tmpArr.join('');
};
//# sourceMappingURL=utf8_decode.js.map