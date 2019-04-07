'use strict';

module.exports = function substr_replace(str, replace, start, length) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/substr_replace/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: substr_replace('ABCDEFGH:/MNRPQR/', 'bob', 0)
  //   returns 1: 'bob'
  //   example 2: var $var = 'ABCDEFGH:/MNRPQR/'
  //   example 2: substr_replace($var, 'bob', 0, $var.length)
  //   returns 2: 'bob'
  //   example 3: substr_replace('ABCDEFGH:/MNRPQR/', 'bob', 0, 0)
  //   returns 3: 'bobABCDEFGH:/MNRPQR/'
  //   example 4: substr_replace('ABCDEFGH:/MNRPQR/', 'bob', 10, -1)
  //   returns 4: 'ABCDEFGH:/bob/'
  //   example 5: substr_replace('ABCDEFGH:/MNRPQR/', 'bob', -7, -1)
  //   returns 5: 'ABCDEFGH:/bob/'
  //   example 6: substr_replace('ABCDEFGH:/MNRPQR/', '', 10, -1)
  //   returns 6: 'ABCDEFGH://'

  if (start < 0) {
    // start position in str
    start = start + str.length;
  }
  length = length !== undefined ? length : str.length;
  if (length < 0) {
    length = length + str.length - start;
  }

  return [str.slice(0, start), replace.substr(0, length), replace.slice(length), str.slice(start + length)].join('');
};
//# sourceMappingURL=substr_replace.js.map