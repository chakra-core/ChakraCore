'use strict';

module.exports = function chunk_split(body, chunklen, end) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/chunk_split/
  // original by: Paulo Freitas
  //    input by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Theriault (https://github.com/Theriault)
  //   example 1: chunk_split('Hello world!', 1, '*')
  //   returns 1: 'H*e*l*l*o* *w*o*r*l*d*!*'
  //   example 2: chunk_split('Hello world!', 10, '*')
  //   returns 2: 'Hello worl*d!*'

  chunklen = parseInt(chunklen, 10) || 76;
  end = end || '\r\n';

  if (chunklen < 1) {
    return false;
  }

  return body.match(new RegExp('.{0,' + chunklen + '}', 'g')).join(end);
};
//# sourceMappingURL=chunk_split.js.map