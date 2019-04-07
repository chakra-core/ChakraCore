'use strict';

module.exports = function str_split(string, splitLength) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/str_split/
  // original by: Martijn Wieringa
  // improved by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  //  revised by: Theriault (https://github.com/Theriault)
  //  revised by: Rafał Kukawski (http://blog.kukawski.pl)
  //    input by: Bjorn Roesbeke (http://www.bjornroesbeke.be/)
  //   example 1: str_split('Hello Friend', 3)
  //   returns 1: ['Hel', 'lo ', 'Fri', 'end']

  if (splitLength === null) {
    splitLength = 1;
  }
  if (string === null || splitLength < 1) {
    return false;
  }

  string += '';
  var chunks = [];
  var pos = 0;
  var len = string.length;

  while (pos < len) {
    chunks.push(string.slice(pos, pos += splitLength));
  }

  return chunks;
};
//# sourceMappingURL=str_split.js.map