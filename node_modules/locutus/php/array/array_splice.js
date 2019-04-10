'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function array_splice(arr, offst, lgth, replacement) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_splice/
  // original by: Brett Zamir (http://brett-zamir.me)
  //    input by: Theriault (https://github.com/Theriault)
  //      note 1: Order does get shifted in associative array input with numeric indices,
  //      note 1: since PHP behavior doesn't preserve keys, but I understand order is
  //      note 1: not reliable anyways
  //      note 1: Note also that IE retains information about property position even
  //      note 1: after being supposedly deleted, so use of this function may produce
  //      note 1: unexpected results in IE if you later attempt to add back properties
  //      note 1: with the same keys that had been deleted
  //   example 1: var $input = {4: "red", 'abc': "green", 2: "blue", 'dud': "yellow"}
  //   example 1: array_splice($input, 2)
  //   returns 1: {4: "red", 'abc': "green"}
  //   example 2: var $input = ["red", "green", "blue", "yellow"]
  //   example 2: array_splice($input, 3, 0, "purple")
  //   returns 2: []
  //   example 3: var $input = ["red", "green", "blue", "yellow"]
  //   example 3: array_splice($input, -1, 1, ["black", "maroon"])
  //   returns 3: ["yellow"]
  //        test: skip-1

  var isInt = require('../var/is_int');

  var _checkToUpIndices = function _checkToUpIndices(arr, ct, key) {
    // Deal with situation, e.g., if encounter index 4 and try
    // to set it to 0, but 0 exists later in loop (need to
    // increment all subsequent (skipping current key,
    // since we need its value below) until find unused)
    if (arr[ct] !== undefined) {
      var tmp = ct;
      ct += 1;
      if (ct === key) {
        ct += 1;
      }
      ct = _checkToUpIndices(arr, ct, key);
      arr[ct] = arr[tmp];
      delete arr[tmp];
    }
    return ct;
  };

  if (replacement && (typeof replacement === 'undefined' ? 'undefined' : _typeof(replacement)) !== 'object') {
    replacement = [replacement];
  }
  if (lgth === undefined) {
    lgth = offst >= 0 ? arr.length - offst : -offst;
  } else if (lgth < 0) {
    lgth = (offst >= 0 ? arr.length - offst : -offst) + lgth;
  }

  if (Object.prototype.toString.call(arr) !== '[object Array]') {
    /* if (arr.length !== undefined) {
     // Deal with array-like objects as input
    delete arr.length;
    } */
    var lgt = 0;
    var ct = -1;
    var rmvd = [];
    var rmvdObj = {};
    var replCt = -1;
    var intCt = -1;
    var returnArr = true;
    var rmvdCt = 0;
    // var rmvdLngth = 0
    var key = '';
    // rmvdObj.length = 0;
    for (key in arr) {
      // Can do arr.__count__ in some browsers
      lgt += 1;
    }
    offst = offst >= 0 ? offst : lgt + offst;
    for (key in arr) {
      ct += 1;
      if (ct < offst) {
        if (isInt(key)) {
          intCt += 1;
          if (parseInt(key, 10) === intCt) {
            // Key is already numbered ok, so don't need to change key for value
            continue;
          }
          // Deal with situation, e.g.,
          _checkToUpIndices(arr, intCt, key);
          // if encounter index 4 and try to set it to 0, but 0 exists later in loop
          arr[intCt] = arr[key];
          delete arr[key];
        }
        continue;
      }
      if (returnArr && isInt(key)) {
        rmvd.push(arr[key]);
        // PHP starts over here too
        rmvdObj[rmvdCt++] = arr[key];
      } else {
        rmvdObj[key] = arr[key];
        returnArr = false;
      }
      // rmvdLngth += 1
      // rmvdObj.length += 1;
      if (replacement && replacement[++replCt]) {
        arr[key] = replacement[replCt];
      } else {
        delete arr[key];
      }
    }
    // Make (back) into an array-like object
    // arr.length = lgt - rmvdLngth + (replacement ? replacement.length : 0);
    return returnArr ? rmvd : rmvdObj;
  }

  if (replacement) {
    replacement.unshift(offst, lgth);
    return Array.prototype.splice.apply(arr, replacement);
  }

  return arr.splice(offst, lgth);
};
//# sourceMappingURL=array_splice.js.map