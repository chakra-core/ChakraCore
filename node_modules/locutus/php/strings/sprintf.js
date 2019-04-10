'use strict';

module.exports = function sprintf() {
  //  discuss at: http://locutus.io/php/sprintf/
  // original by: Ash Searle (http://hexmen.com/blog/)
  // improved by: Michael White (http://getsprink.com)
  // improved by: Jack
  // improved by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Dj
  // improved by: Allidylls
  //    input by: Paulo Freitas
  //    input by: Brett Zamir (http://brett-zamir.me)
  // improved by: Rafał Kukawski (http://kukawski.pl)
  //   example 1: sprintf("%01.2f", 123.1)
  //   returns 1: '123.10'
  //   example 2: sprintf("[%10s]", 'monkey')
  //   returns 2: '[    monkey]'
  //   example 3: sprintf("[%'#10s]", 'monkey')
  //   returns 3: '[####monkey]'
  //   example 4: sprintf("%d", 123456789012345)
  //   returns 4: '123456789012345'
  //   example 5: sprintf('%-03s', 'E')
  //   returns 5: 'E00'
  //   example 6: sprintf('%+010d', 9)
  //   returns 6: '+000000009'
  //   example 7: sprintf('%+0\'@10d', 9)
  //   returns 7: '@@@@@@@@+9'
  //   example 8: sprintf('%.f', 3.14)
  //   returns 8: '3.140000'
  //   example 9: sprintf('%% %2$d', 1, 2)
  //   returns 9: '% 2'

  var regex = /%%|%(?:(\d+)\$)?((?:[-+#0 ]|'[\s\S])*)(\d+)?(?:\.(\d*))?([\s\S])/g;
  var args = arguments;
  var i = 0;
  var format = args[i++];

  var _pad = function _pad(str, len, chr, leftJustify) {
    if (!chr) {
      chr = ' ';
    }
    var padding = str.length >= len ? '' : new Array(1 + len - str.length >>> 0).join(chr);
    return leftJustify ? str + padding : padding + str;
  };

  var justify = function justify(value, prefix, leftJustify, minWidth, padChar) {
    var diff = minWidth - value.length;
    if (diff > 0) {
      // when padding with zeros
      // on the left side
      // keep sign (+ or -) in front
      if (!leftJustify && padChar === '0') {
        value = [value.slice(0, prefix.length), _pad('', diff, '0', true), value.slice(prefix.length)].join('');
      } else {
        value = _pad(value, minWidth, padChar, leftJustify);
      }
    }
    return value;
  };

  var _formatBaseX = function _formatBaseX(value, base, leftJustify, minWidth, precision, padChar) {
    // Note: casts negative numbers to positive ones
    var number = value >>> 0;
    value = _pad(number.toString(base), precision || 0, '0', false);
    return justify(value, '', leftJustify, minWidth, padChar);
  };

  // _formatString()
  var _formatString = function _formatString(value, leftJustify, minWidth, precision, customPadChar) {
    if (precision !== null && precision !== undefined) {
      value = value.slice(0, precision);
    }
    return justify(value, '', leftJustify, minWidth, customPadChar);
  };

  // doFormat()
  var doFormat = function doFormat(substring, argIndex, modifiers, minWidth, precision, specifier) {
    var number, prefix, method, textTransform, value;

    if (substring === '%%') {
      return '%';
    }

    // parse modifiers
    var padChar = ' '; // pad with spaces by default
    var leftJustify = false;
    var positiveNumberPrefix = '';
    var j, l;

    for (j = 0, l = modifiers.length; j < l; j++) {
      switch (modifiers.charAt(j)) {
        case ' ':
        case '0':
          padChar = modifiers.charAt(j);
          break;
        case '+':
          positiveNumberPrefix = '+';
          break;
        case '-':
          leftJustify = true;
          break;
        case "'":
          if (j + 1 < l) {
            padChar = modifiers.charAt(j + 1);
            j++;
          }
          break;
      }
    }

    if (!minWidth) {
      minWidth = 0;
    } else {
      minWidth = +minWidth;
    }

    if (!isFinite(minWidth)) {
      throw new Error('Width must be finite');
    }

    if (!precision) {
      precision = specifier === 'd' ? 0 : 'fFeE'.indexOf(specifier) > -1 ? 6 : undefined;
    } else {
      precision = +precision;
    }

    if (argIndex && +argIndex === 0) {
      throw new Error('Argument number must be greater than zero');
    }

    if (argIndex && +argIndex >= args.length) {
      throw new Error('Too few arguments');
    }

    value = argIndex ? args[+argIndex] : args[i++];

    switch (specifier) {
      case '%':
        return '%';
      case 's':
        return _formatString(value + '', leftJustify, minWidth, precision, padChar);
      case 'c':
        return _formatString(String.fromCharCode(+value), leftJustify, minWidth, precision, padChar);
      case 'b':
        return _formatBaseX(value, 2, leftJustify, minWidth, precision, padChar);
      case 'o':
        return _formatBaseX(value, 8, leftJustify, minWidth, precision, padChar);
      case 'x':
        return _formatBaseX(value, 16, leftJustify, minWidth, precision, padChar);
      case 'X':
        return _formatBaseX(value, 16, leftJustify, minWidth, precision, padChar).toUpperCase();
      case 'u':
        return _formatBaseX(value, 10, leftJustify, minWidth, precision, padChar);
      case 'i':
      case 'd':
        number = +value || 0;
        // Plain Math.round doesn't just truncate
        number = Math.round(number - number % 1);
        prefix = number < 0 ? '-' : positiveNumberPrefix;
        value = prefix + _pad(String(Math.abs(number)), precision, '0', false);

        if (leftJustify && padChar === '0') {
          // can't right-pad 0s on integers
          padChar = ' ';
        }
        return justify(value, prefix, leftJustify, minWidth, padChar);
      case 'e':
      case 'E':
      case 'f': // @todo: Should handle locales (as per setlocale)
      case 'F':
      case 'g':
      case 'G':
        number = +value;
        prefix = number < 0 ? '-' : positiveNumberPrefix;
        method = ['toExponential', 'toFixed', 'toPrecision']['efg'.indexOf(specifier.toLowerCase())];
        textTransform = ['toString', 'toUpperCase']['eEfFgG'.indexOf(specifier) % 2];
        value = prefix + Math.abs(number)[method](precision);
        return justify(value, prefix, leftJustify, minWidth, padChar)[textTransform]();
      default:
        // unknown specifier, consume that char and return empty
        return '';
    }
  };

  try {
    return format.replace(regex, doFormat);
  } catch (err) {
    return false;
  }
};
//# sourceMappingURL=sprintf.js.map