'use strict';

module.exports = function str_word_count(str, format, charlist) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/str_word_count/
  // original by: Ole Vrijenhoek
  // bugfixed by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  //    input by: Bug?
  // improved by: Brett Zamir (http://brett-zamir.me)
  //   example 1: str_word_count("Hello fri3nd, you're\r\n       looking          good today!", 1)
  //   returns 1: ['Hello', 'fri', 'nd', "you're", 'looking', 'good', 'today']
  //   example 2: str_word_count("Hello fri3nd, you're\r\n       looking          good today!", 2)
  //   returns 2: {0: 'Hello', 6: 'fri', 10: 'nd', 14: "you're", 29: 'looking', 46: 'good', 51: 'today'}
  //   example 3: str_word_count("Hello fri3nd, you're\r\n       looking          good today!", 1, '\u00e0\u00e1\u00e3\u00e73')
  //   returns 3: ['Hello', 'fri3nd', "you're", 'looking', 'good', 'today']
  //   example 4: str_word_count('hey', 2)
  //   returns 4: {0: 'hey'}

  var ctypeAlpha = require('../ctype/ctype_alpha');
  var len = str.length;
  var cl = charlist && charlist.length;
  var chr = '';
  var tmpStr = '';
  var i = 0;
  var c = '';
  var wArr = [];
  var wC = 0;
  var assoc = {};
  var aC = 0;
  var reg = '';
  var match = false;

  var _pregQuote = function _pregQuote(str) {
    return (str + '').replace(/([\\.+*?[^\]$(){}=!<>|:])/g, '\\$1');
  };
  var _getWholeChar = function _getWholeChar(str, i) {
    // Use for rare cases of non-BMP characters
    var code = str.charCodeAt(i);
    if (code < 0xD800 || code > 0xDFFF) {
      return str.charAt(i);
    }
    if (code >= 0xD800 && code <= 0xDBFF) {
      // High surrogate (could change last hex to 0xDB7F to treat high private surrogates as single
      // characters)
      if (str.length <= i + 1) {
        throw new Error('High surrogate without following low surrogate');
      }
      var next = str.charCodeAt(i + 1);
      if (next < 0xDC00 || next > 0xDFFF) {
        throw new Error('High surrogate without following low surrogate');
      }
      return str.charAt(i) + str.charAt(i + 1);
    }
    // Low surrogate (0xDC00 <= code && code <= 0xDFFF)
    if (i === 0) {
      throw new Error('Low surrogate without preceding high surrogate');
    }
    var prev = str.charCodeAt(i - 1);
    if (prev < 0xD800 || prev > 0xDBFF) {
      // (could change last hex to 0xDB7F to treat high private surrogates as single characters)
      throw new Error('Low surrogate without preceding high surrogate');
    }
    // We can pass over low surrogates now as the second component in a pair which we have already
    // processed
    return false;
  };

  if (cl) {
    reg = '^(' + _pregQuote(_getWholeChar(charlist, 0));
    for (i = 1; i < cl; i++) {
      if ((chr = _getWholeChar(charlist, i)) === false) {
        continue;
      }
      reg += '|' + _pregQuote(chr);
    }
    reg += ')$';
    reg = new RegExp(reg);
  }

  for (i = 0; i < len; i++) {
    if ((c = _getWholeChar(str, i)) === false) {
      continue;
    }
    // No hyphen at beginning or end unless allowed in charlist (or locale)
    // No apostrophe at beginning unless allowed in charlist (or locale)
    // @todo: Make this more readable
    match = ctypeAlpha(c) || reg && c.search(reg) !== -1 || i !== 0 && i !== len - 1 && c === '-' || i !== 0 && c === "'";
    if (match) {
      if (tmpStr === '' && format === 2) {
        aC = i;
      }
      tmpStr = tmpStr + c;
    }
    if (i === len - 1 || !match && tmpStr !== '') {
      if (format !== 2) {
        wArr[wArr.length] = tmpStr;
      } else {
        assoc[aC] = tmpStr;
      }
      tmpStr = '';
      wC++;
    }
  }

  if (!format) {
    return wC;
  } else if (format === 1) {
    return wArr;
  } else if (format === 2) {
    return assoc;
  }

  throw new Error('You have supplied an incorrect format');
};
//# sourceMappingURL=str_word_count.js.map