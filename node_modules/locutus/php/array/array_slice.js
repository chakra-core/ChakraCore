'use strict';

module.exports = function array_slice(arr, offst, lgth, preserveKeys) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_slice/
  // original by: Brett Zamir (http://brett-zamir.me)
  //    input by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: Kevin van Zonneveld (http://kvz.io)
  //      note 1: Relies on is_int because !isNaN accepts floats
  //   example 1: array_slice(["a", "b", "c", "d", "e"], 2, -1)
  //   returns 1: [ 'c', 'd' ]
  //   example 2: array_slice(["a", "b", "c", "d", "e"], 2, -1, true)
  //   returns 2: {2: 'c', 3: 'd'}

  var isInt = require('../var/is_int');

  /*
    if ('callee' in arr && 'length' in arr) {
      arr = Array.prototype.slice.call(arr);
    }
  */

  var key = '';

  if (Object.prototype.toString.call(arr) !== '[object Array]' || preserveKeys && offst !== 0) {
    // Assoc. array as input or if required as output
    var lgt = 0;
    var newAssoc = {};
    for (key in arr) {
      lgt += 1;
      newAssoc[key] = arr[key];
    }
    arr = newAssoc;

    offst = offst < 0 ? lgt + offst : offst;
    lgth = lgth === undefined ? lgt : lgth < 0 ? lgt + lgth - offst : lgth;

    var assoc = {};
    var start = false;
    var it = -1;
    var arrlgth = 0;
    var noPkIdx = 0;

    for (key in arr) {
      ++it;
      if (arrlgth >= lgth) {
        break;
      }
      if (it === offst) {
        start = true;
      }
      if (!start) {
        continue;
      }++arrlgth;
      if (isInt(key) && !preserveKeys) {
        assoc[noPkIdx++] = arr[key];
      } else {
        assoc[key] = arr[key];
      }
    }
    // Make as array-like object (though length will not be dynamic)
    // assoc.length = arrlgth;
    return assoc;
  }

  if (lgth === undefined) {
    return arr.slice(offst);
  } else if (lgth >= 0) {
    return arr.slice(offst, offst + lgth);
  } else {
    return arr.slice(offst, lgth);
  }
};
//# sourceMappingURL=array_slice.js.map