'use strict';

module.exports = function array_replace(arr) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_replace/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: array_replace(["orange", "banana", "apple", "raspberry"], {0 : "pineapple", 4 : "cherry"}, {0:"grape"})
  //   returns 1: {0: 'grape', 1: 'banana', 2: 'apple', 3: 'raspberry', 4: 'cherry'}

  var retObj = {};
  var i = 0;
  var p = '';
  var argl = arguments.length;

  if (argl < 2) {
    throw new Error('There should be at least 2 arguments passed to array_replace()');
  }

  // Although docs state that the arguments are passed in by reference,
  // it seems they are not altered, but rather the copy that is returned
  // (just guessing), so we make a copy here, instead of acting on arr itself
  for (p in arr) {
    retObj[p] = arr[p];
  }

  for (i = 1; i < argl; i++) {
    for (p in arguments[i]) {
      retObj[p] = arguments[i][p];
    }
  }

  return retObj;
};
//# sourceMappingURL=array_replace.js.map