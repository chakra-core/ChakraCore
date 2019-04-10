'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function array_combine(keys, values) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_combine/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //   example 1: array_combine([0,1,2], ['kevin','van','zonneveld'])
  //   returns 1: {0: 'kevin', 1: 'van', 2: 'zonneveld'}

  var newArray = {};
  var i = 0;

  // input sanitation
  // Only accept arrays or array-like objects
  // Require arrays to have a count
  if ((typeof keys === 'undefined' ? 'undefined' : _typeof(keys)) !== 'object') {
    return false;
  }
  if ((typeof values === 'undefined' ? 'undefined' : _typeof(values)) !== 'object') {
    return false;
  }
  if (typeof keys.length !== 'number') {
    return false;
  }
  if (typeof values.length !== 'number') {
    return false;
  }
  if (!keys.length) {
    return false;
  }

  // number of elements does not match
  if (keys.length !== values.length) {
    return false;
  }

  for (i = 0; i < keys.length; i++) {
    newArray[keys[i]] = values[i];
  }

  return newArray;
};
//# sourceMappingURL=array_combine.js.map