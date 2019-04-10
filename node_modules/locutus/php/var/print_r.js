'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function print_r(array, returnVal) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/print_r/
  // original by: Michael White (http://getsprink.com)
  // improved by: Ben Bryan
  // improved by: Brett Zamir (http://brett-zamir.me)
  // improved by: Kevin van Zonneveld (http://kvz.io)
  //    input by: Brett Zamir (http://brett-zamir.me)
  //   example 1: print_r(1, true)
  //   returns 1: '1'

  var echo = require('../strings/echo');

  var output = '';
  var padChar = ' ';
  var padVal = 4;

  var _repeatChar = function _repeatChar(len, padChar) {
    var str = '';
    for (var i = 0; i < len; i++) {
      str += padChar;
    }
    return str;
  };
  var _formatArray = function _formatArray(obj, curDepth, padVal, padChar) {
    if (curDepth > 0) {
      curDepth++;
    }

    var basePad = _repeatChar(padVal * curDepth, padChar);
    var thickPad = _repeatChar(padVal * (curDepth + 1), padChar);
    var str = '';

    if ((typeof obj === 'undefined' ? 'undefined' : _typeof(obj)) === 'object' && obj !== null && obj.constructor) {
      str += 'Array\n' + basePad + '(\n';
      for (var key in obj) {
        if (Object.prototype.toString.call(obj[key]) === '[object Array]') {
          str += thickPad;
          str += '[';
          str += key;
          str += '] => ';
          str += _formatArray(obj[key], curDepth + 1, padVal, padChar);
        } else {
          str += thickPad;
          str += '[';
          str += key;
          str += '] => ';
          str += obj[key];
          str += '\n';
        }
      }
      str += basePad + ')\n';
    } else if (obj === null || obj === undefined) {
      str = '';
    } else {
      // for our "resource" class
      str = obj.toString();
    }

    return str;
  };

  output = _formatArray(array, 0, padVal, padChar);

  if (returnVal !== true) {
    echo(output);
    return true;
  }
  return output;
};
//# sourceMappingURL=print_r.js.map