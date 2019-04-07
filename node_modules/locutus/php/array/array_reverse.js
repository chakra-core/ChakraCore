'use strict';

module.exports = function array_reverse(array, preserveKeys) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_reverse/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Karol Kowalski
  //   example 1: array_reverse( [ 'php', '4.0', ['green', 'red'] ], true)
  //   returns 1: { 2: ['green', 'red'], 1: '4.0', 0: 'php'}

  var isArray = Object.prototype.toString.call(array) === '[object Array]';
  var tmpArr = preserveKeys ? {} : [];
  var key;

  if (isArray && !preserveKeys) {
    return array.slice(0).reverse();
  }

  if (preserveKeys) {
    var keys = [];
    for (key in array) {
      keys.push(key);
    }

    var i = keys.length;
    while (i--) {
      key = keys[i];
      // @todo: don't rely on browsers keeping keys in insertion order
      // it's implementation specific
      // eg. the result will differ from expected in Google Chrome
      tmpArr[key] = array[key];
    }
  } else {
    for (key in array) {
      tmpArr.unshift(array[key]);
    }
  }

  return tmpArr;
};
//# sourceMappingURL=array_reverse.js.map