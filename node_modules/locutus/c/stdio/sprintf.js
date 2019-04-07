'use strict';

function pad(str, minLength, padChar, leftJustify) {
  var diff = minLength - str.length;
  var padStr = padChar.repeat(Math.max(0, diff));

  return leftJustify ? str + padStr : padStr + str;
}

module.exports = function sprintf(format) {
  for (var _len = arguments.length, args = Array(_len > 1 ? _len - 1 : 0), _key = 1; _key < _len; _key++) {
    args[_key - 1] = arguments[_key];
  }

  // original by: Rafał Kukawski
  //   example 1: sprintf('%+10.*d', 5, 1)
  //   returns 1: '    +00001'
  var placeholderRegex = /%(?:(\d+)\$)?([-+#0 ]*)(\*|\d+)?(?:\.(\*|\d*))?([\s\S])/g;

  var index = 0;

  return format.replace(placeholderRegex, function (match, param, flags, width, prec, modifier) {
    var leftJustify = flags.includes('-');

    // flag '0' is ignored when flag '-' is present
    var padChar = leftJustify ? ' ' : flags.split('').reduce(function (pc, c) {
      return [' ', '0'].includes(c) ? c : pc;
    }, ' ');

    var positiveSign = flags.includes('+') ? '+' : flags.includes(' ') ? ' ' : '';

    var minWidth = width === '*' ? args[index++] : +width || 0;
    var precision = prec === '*' ? args[index++] : +prec;

    if (param && !+param) {
      throw Error('Param index must be greater than 0');
    }

    if (param && +param > args.length) {
      throw Error('Too few arguments');
    }

    // compiling with default clang params, mixed positional and non-positional params
    // give only a warning
    var arg = param ? args[param - 1] : args[index++];

    if (precision === undefined || isNaN(precision)) {
      precision = 'eEfFgG'.includes(modifier) ? 6 : modifier === 's' ? String(arg).length : undefined;
    }

    switch (modifier) {
      case '%':
        return '%';
      case 'd':
      case 'i':
        {
          var number = Math.trunc(+arg || 0);
          var abs = Math.abs(number);
          var prefix = number < 0 ? '-' : positiveSign;

          var str = pad(abs.toString(), precision || 0, '0', false);

          if (padChar === '0') {
            return prefix + pad(str, minWidth - prefix.length, padChar, leftJustify);
          }

          return pad(prefix + str, minWidth, padChar, leftJustify);
        }
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
        {
          var _number = +arg;
          var _abs = Math.abs(_number);
          var _prefix = _number < 0 ? '-' : positiveSign;

          var op = [Number.prototype.toExponential, Number.prototype.toFixed, Number.prototype.toPrecision]['efg'.indexOf(modifier.toLowerCase())];

          var tr = [String.prototype.toLowerCase, String.prototype.toUpperCase]['eEfFgG'.indexOf(modifier) % 2];

          var isSpecial = isNaN(_abs) || !isFinite(_abs);

          var _str = isSpecial ? _abs.toString().substr(0, 3) : op.call(_abs, precision);

          if (padChar === '0' && !isSpecial) {
            return _prefix + pad(tr.call(_str), minWidth - _prefix.length, padChar, leftJustify);
          }

          return pad(tr.call(_prefix + _str), minWidth, isSpecial ? ' ' : padChar, leftJustify);
        }
      case 'b':
      case 'o':
      case 'u':
      case 'x':
      case 'X':
        {
          var _number2 = +arg || 0;
          var intVal = Math.trunc(_number2) + (_number2 < 0 ? 0xFFFFFFFF + 1 : 0);
          var base = [2, 8, 10, 16, 16]['bouxX'.indexOf(modifier)];
          var _prefix2 = intVal && flags.includes('#') ? ['', '0', '', '0x', '0X']['bouxXX'.indexOf(modifier)] : '';

          if (padChar === '0' && _prefix2) {
            return _prefix2 + pad(pad(intVal.toString(base), precision, '0', false), minWidth - _prefix2.length, padChar, leftJustify);
          }

          return pad(_prefix2 + pad(intVal.toString(base), precision, '0', false), minWidth, padChar, leftJustify);
        }
      case 'p':
      case 'n':
        {
          throw Error('\'' + modifier + '\' modifier not supported');
        }
      case 's':
        {
          return pad(String(arg).substr(0, precision), minWidth, padChar, leftJustify);
        }
      case 'c':
        {
          // extension, if arg is string, take first char
          var chr = typeof arg === 'string' ? arg.charAt(0) : String.fromCharCode(+arg);
          return pad(chr, minWidth, padChar, leftJustify);
        }
      case 'a':
      case 'A':
        throw Error('\'' + modifier + '\' modifier not yet implemented');
      default:
        // for unknown modifiers, return the modifier char
        return modifier;
    }
  });
};
//# sourceMappingURL=sprintf.js.map