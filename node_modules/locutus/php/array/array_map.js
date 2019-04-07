'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function array_map(callback) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_map/
  // original by: Andrea Giammarchi (http://webreflection.blogspot.com)
  // improved by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //    input by: thekid
  //      note 1: If the callback is a string (or object, if an array is supplied),
  //      note 1: it can only work if the function name is in the global context
  //   example 1: array_map( function (a){return (a * a * a)}, [1, 2, 3, 4, 5] )
  //   returns 1: [ 1, 8, 27, 64, 125 ]

  var argc = arguments.length;
  var argv = arguments;
  var obj = null;
  var cb = callback;
  var j = argv[1].length;
  var i = 0;
  var k = 1;
  var m = 0;
  var tmp = [];
  var tmpArr = [];

  var $global = typeof window !== 'undefined' ? window : global;

  while (i < j) {
    while (k < argc) {
      tmp[m++] = argv[k++][i];
    }

    m = 0;
    k = 1;

    if (callback) {
      if (typeof callback === 'string') {
        cb = $global[callback];
      } else if ((typeof callback === 'undefined' ? 'undefined' : _typeof(callback)) === 'object' && callback.length) {
        obj = typeof callback[0] === 'string' ? $global[callback[0]] : callback[0];
        if (typeof obj === 'undefined') {
          throw new Error('Object not found: ' + callback[0]);
        }
        cb = typeof callback[1] === 'string' ? obj[callback[1]] : callback[1];
      }
      tmpArr[i++] = cb.apply(obj, tmp);
    } else {
      tmpArr[i++] = tmp;
    }

    tmp = [];
  }

  return tmpArr;
};
//# sourceMappingURL=array_map.js.map