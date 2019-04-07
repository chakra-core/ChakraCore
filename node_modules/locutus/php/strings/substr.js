'use strict';

module.exports = function substr(str, start, len) {
  //  discuss at: http://locutus.io/php/substr/
  // original by: Martijn Wieringa
  // bugfixed by: T.Wild
  // improved by: Onno Marsman (https://twitter.com/onnomarsman)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //  revised by: Theriault (https://github.com/Theriault)
  //      note 1: Handles rare Unicode characters if 'unicode.semantics' ini (PHP6) is set to 'on'
  //   example 1: substr('abcdef', 0, -1)
  //   returns 1: 'abcde'
  //   example 2: substr(2, 0, -6)
  //   returns 2: false
  //   example 3: ini_set('unicode.semantics', 'on')
  //   example 3: substr('a\uD801\uDC00', 0, -1)
  //   returns 3: 'a'
  //   example 4: ini_set('unicode.semantics', 'on')
  //   example 4: substr('a\uD801\uDC00', 0, 2)
  //   returns 4: 'a\uD801\uDC00'
  //   example 5: ini_set('unicode.semantics', 'on')
  //   example 5: substr('a\uD801\uDC00', -1, 1)
  //   returns 5: '\uD801\uDC00'
  //   example 6: ini_set('unicode.semantics', 'on')
  //   example 6: substr('a\uD801\uDC00z\uD801\uDC00', -3, 2)
  //   returns 6: '\uD801\uDC00z'
  //   example 7: ini_set('unicode.semantics', 'on')
  //   example 7: substr('a\uD801\uDC00z\uD801\uDC00', -3, -1)
  //   returns 7: '\uD801\uDC00z'
  //        test: skip-3 skip-4 skip-5 skip-6 skip-7

  str += '';
  var end = str.length;

  var iniVal = (typeof require !== 'undefined' ? require('../info/ini_get')('unicode.emantics') : undefined) || 'off';

  if (iniVal === 'off') {
    // assumes there are no non-BMP characters;
    // if there may be such characters, then it is best to turn it on (critical in true XHTML/XML)
    if (start < 0) {
      start += end;
    }
    if (typeof len !== 'undefined') {
      if (len < 0) {
        end = len + end;
      } else {
        end = len + start;
      }
    }

    // PHP returns false if start does not fall within the string.
    // PHP returns false if the calculated end comes before the calculated start.
    // PHP returns an empty string if start and end are the same.
    // Otherwise, PHP returns the portion of the string from start to end.
    if (start >= str.length || start < 0 || start > end) {
      return false;
    }

    return str.slice(start, end);
  }

  // Full-blown Unicode including non-Basic-Multilingual-Plane characters
  var i = 0;
  var allBMP = true;
  var es = 0;
  var el = 0;
  var se = 0;
  var ret = '';

  for (i = 0; i < str.length; i++) {
    if (/[\uD800-\uDBFF]/.test(str.charAt(i)) && /[\uDC00-\uDFFF]/.test(str.charAt(i + 1))) {
      allBMP = false;
      break;
    }
  }

  if (!allBMP) {
    if (start < 0) {
      for (i = end - 1, es = start += end; i >= es; i--) {
        if (/[\uDC00-\uDFFF]/.test(str.charAt(i)) && /[\uD800-\uDBFF]/.test(str.charAt(i - 1))) {
          start--;
          es--;
        }
      }
    } else {
      var surrogatePairs = /[\uD800-\uDBFF][\uDC00-\uDFFF]/g;
      while (surrogatePairs.exec(str) !== null) {
        var li = surrogatePairs.lastIndex;
        if (li - 2 < start) {
          start++;
        } else {
          break;
        }
      }
    }

    if (start >= end || start < 0) {
      return false;
    }
    if (len < 0) {
      for (i = end - 1, el = end += len; i >= el; i--) {
        if (/[\uDC00-\uDFFF]/.test(str.charAt(i)) && /[\uD800-\uDBFF]/.test(str.charAt(i - 1))) {
          end--;
          el--;
        }
      }
      if (start > end) {
        return false;
      }
      return str.slice(start, end);
    } else {
      se = start + len;
      for (i = start; i < se; i++) {
        ret += str.charAt(i);
        if (/[\uD800-\uDBFF]/.test(str.charAt(i)) && /[\uDC00-\uDFFF]/.test(str.charAt(i + 1))) {
          // Go one further, since one of the "characters" is part of a surrogate pair
          se++;
        }
      }
      return ret;
    }
  }
};
//# sourceMappingURL=substr.js.map