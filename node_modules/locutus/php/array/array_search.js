'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function array_search(needle, haystack, argStrict) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_search/
  // original by: Kevin van Zonneveld (http://kvz.io)
  //    input by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Reynier de la Rosa (http://scriptinside.blogspot.com.es/)
  //        test: skip-all
  //   example 1: array_search('zonneveld', {firstname: 'kevin', middle: 'van', surname: 'zonneveld'})
  //   returns 1: 'surname'
  //   example 2: array_search('3', {a: 3, b: 5, c: 7})
  //   returns 2: 'a'

  var strict = !!argStrict;
  var key = '';

  if ((typeof needle === 'undefined' ? 'undefined' : _typeof(needle)) === 'object' && needle.exec) {
    // Duck-type for RegExp
    if (!strict) {
      // Let's consider case sensitive searches as strict
      var flags = 'i' + (needle.global ? 'g' : '') + (needle.multiline ? 'm' : '') + (
      // sticky is FF only
      needle.sticky ? 'y' : '');
      needle = new RegExp(needle.source, flags);
    }
    for (key in haystack) {
      if (haystack.hasOwnProperty(key)) {
        if (needle.test(haystack[key])) {
          return key;
        }
      }
    }
    return false;
  }

  for (key in haystack) {
    if (haystack.hasOwnProperty(key)) {
      if (strict && haystack[key] === needle || !strict && haystack[key] == needle) {
        // eslint-disable-line eqeqeq
        return key;
      }
    }
  }

  return false;
};
//# sourceMappingURL=array_search.js.map