'use strict';

module.exports = function array_push(inputArr) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_push/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //      note 1: Note also that IE retains information about property position even
  //      note 1: after being supposedly deleted, so if you delete properties and then
  //      note 1: add back properties with the same keys (including numeric) that had
  //      note 1: been deleted, the order will be as before; thus, this function is not
  //      note 1: really recommended with associative arrays (objects) in IE environments
  //   example 1: array_push(['kevin','van'], 'zonneveld')
  //   returns 1: 3

  var i = 0;
  var pr = '';
  var argv = arguments;
  var argc = argv.length;
  var allDigits = /^\d$/;
  var size = 0;
  var highestIdx = 0;
  var len = 0;

  if (inputArr.hasOwnProperty('length')) {
    for (i = 1; i < argc; i++) {
      inputArr[inputArr.length] = argv[i];
    }
    return inputArr.length;
  }

  // Associative (object)
  for (pr in inputArr) {
    if (inputArr.hasOwnProperty(pr)) {
      ++len;
      if (pr.search(allDigits) !== -1) {
        size = parseInt(pr, 10);
        highestIdx = size > highestIdx ? size : highestIdx;
      }
    }
  }
  for (i = 1; i < argc; i++) {
    inputArr[++highestIdx] = argv[i];
  }

  return len + i - 1;
};
//# sourceMappingURL=array_push.js.map