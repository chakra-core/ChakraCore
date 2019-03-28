/*!
 * split-string <https://github.com/jonschlinkert/split-string>
 *
 * Copyright (c) 2015, 2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var extend = require('extend-shallow');

module.exports = function(str, options) {
  if (typeof str !== 'string') {
    throw new TypeError('expected a string');
  }

  // allow separator to be defined as a string
  if (typeof options === 'string') {
    options = {sep: options};
  }

  var opts = extend({sep: '.'}, options);
  var arr = [''];
  var len = str.length;
  var idx = -1;
  var closeIdx;

  while (++idx < len) {
    var substr = str[idx];
    var next = str[idx + 1];

    if (substr === '\\') {
      var val = opts.keepEscaping === true ? (substr + next) : next;
      arr[arr.length - 1] += val;
      idx++;
      continue;

    } else {
      if (substr === '"') {
        closeIdx = getClose(str, '"', idx + 1);
        if (closeIdx === -1) {
          if (opts.strict !== false) {
            throw new Error('unclosed double quote: ' + str);
          }
          closeIdx = idx;
        }

        if (opts.keepDoubleQuotes === true) {
          substr = str.slice(idx, closeIdx + 1);
        } else {
          substr = str.slice(idx + 1, closeIdx);
        }

        idx = closeIdx;
      }

      if (substr === '\'') {
        closeIdx = getClose(str, '\'', idx + 1);
        if (closeIdx === -1) {
          if (opts.strict !== false) {
              throw new Error('unclosed single quote: ' + str);
          }
          closeIdx = idx;
        }

        if (opts.keepSingleQuotes === true) {
          substr = str.slice(idx, closeIdx + 1);
        } else {
          substr = str.slice(idx + 1, closeIdx);
        }

        idx = closeIdx;
      }

      if (substr === opts.sep) {
        arr.push('');
      } else {
        arr[arr.length - 1] += substr;
      }
    }
  }

  return arr;
};

function getClose(str, substr, i) {
  var idx = str.indexOf(substr, i);
  if (str.charAt(idx - 1) === '\\') {
    return getClose(str, substr, idx + 1);
  }
  return idx;
}
