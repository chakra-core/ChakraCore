'use strict';

module.exports = function str_shuffle(str) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/str_shuffle/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: var $shuffled = str_shuffle("abcdef")
  //   example 1: var $result = $shuffled.length
  //   returns 1: 6

  if (arguments.length === 0) {
    throw new Error('Wrong parameter count for str_shuffle()');
  }

  if (str === null) {
    return '';
  }

  str += '';

  var newStr = '';
  var rand;
  var i = str.length;

  while (i) {
    rand = Math.floor(Math.random() * i);
    newStr += str.charAt(rand);
    str = str.substring(0, rand) + str.substr(rand + 1);
    i--;
  }

  return newStr;
};
//# sourceMappingURL=str_shuffle.js.map