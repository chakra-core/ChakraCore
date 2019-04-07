'use strict';

module.exports = function sscanf(str, format) {
  //  discuss at: http://locutus.io/php/sscanf/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: sscanf('SN/2350001', 'SN/%d')
  //   returns 1: [2350001]
  //   example 2: var myVar = {}
  //   example 2: sscanf('SN/2350001', 'SN/%d', myVar)
  //   example 2: var $result = myVar.value
  //   returns 2: 2350001
  //   example 3: sscanf("10--20", "%2$d--%1$d") // Must escape '$' in PHP, but not JS
  //   returns 3: [20, 10]

  var retArr = [];
  var _NWS = /\S/;
  var args = arguments;
  var digit;

  var _setExtraConversionSpecs = function _setExtraConversionSpecs(offset) {
    // Since a mismatched character sets us off track from future
    // legitimate finds, we just scan
    // to the end for any other conversion specifications (besides a percent literal),
    // setting them to null
    // sscanf seems to disallow all conversion specification components (of sprintf)
    // except for type specifiers
    // Do not allow % in last char. class
    // var matches = format.match(/%[+-]?([ 0]|'.)?-?\d*(\.\d+)?[bcdeufFosxX]/g);
    // Do not allow % in last char. class:
    var matches = format.slice(offset).match(/%[cdeEufgosxX]/g);
    // b, F,G give errors in PHP, but 'g', though also disallowed, doesn't
    if (matches) {
      var lgth = matches.length;
      while (lgth--) {
        retArr.push(null);
      }
    }
    return _finish();
  };

  var _finish = function _finish() {
    if (args.length === 2) {
      return retArr;
    }
    for (var i = 0; i < retArr.length; ++i) {
      args[i + 2].value = retArr[i];
    }
    return i;
  };

  var _addNext = function _addNext(j, regex, cb) {
    if (assign) {
      var remaining = str.slice(j);
      var check = width ? remaining.substr(0, width) : remaining;
      var match = regex.exec(check);
      // @todo: Make this more readable
      var key = digit !== undefined ? digit : retArr.length;
      var testNull = retArr[key] = match ? cb ? cb.apply(null, match) : match[0] : null;
      if (testNull === null) {
        throw new Error('No match in string');
      }
      return j + match[0].length;
    }
    return j;
  };

  if (arguments.length < 2) {
    throw new Error('Not enough arguments passed to sscanf');
  }

  // PROCESS
  for (var i = 0, j = 0; i < format.length; i++) {
    var width = 0;
    var assign = true;

    if (format.charAt(i) === '%') {
      if (format.charAt(i + 1) === '%') {
        if (str.charAt(j) === '%') {
          // a matched percent literal
          // skip beyond duplicated percent
          ++i;
          ++j;
          continue;
        }
        // Format indicated a percent literal, but not actually present
        return _setExtraConversionSpecs(i + 2);
      }

      // CHARACTER FOLLOWING PERCENT IS NOT A PERCENT

      // We need 'g' set to get lastIndex
      var prePattern = new RegExp('^(?:(\\d+)\\$)?(\\*)?(\\d*)([hlL]?)', 'g');

      var preConvs = prePattern.exec(format.slice(i + 1));

      var tmpDigit = digit;
      if (tmpDigit && preConvs[1] === undefined) {
        var msg = 'All groups in sscanf() must be expressed as numeric if ';
        msg += 'any have already been used';
        throw new Error(msg);
      }
      digit = preConvs[1] ? parseInt(preConvs[1], 10) - 1 : undefined;

      assign = !preConvs[2];
      width = parseInt(preConvs[3], 10);
      var sizeCode = preConvs[4];
      i += prePattern.lastIndex;

      // @todo: Does PHP do anything with these? Seems not to matter
      if (sizeCode) {
        // This would need to be processed later
        switch (sizeCode) {
          case 'h':
          case 'l':
          case 'L':
            // Treats subsequent as short int (for d,i,n) or unsigned short int (for o,u,x)
            // Treats subsequent as long int (for d,i,n), or unsigned long int (for o,u,x);
            //    or as double (for e,f,g) instead of float or wchar_t instead of char
            // Treats subsequent as long double (for e,f,g)
            break;
          default:
            throw new Error('Unexpected size specifier in sscanf()!');
        }
      }
      // PROCESS CHARACTER
      try {
        // For detailed explanations, see http://web.archive.org/web/20031128125047/http://www.uwm.edu/cgi-bin/IMT/wwwman?topic=scanf%283%29&msection=
        // Also http://www.mathworks.com/access/helpdesk/help/techdoc/ref/sscanf.html
        // p, S, C arguments in C function not available
        // DOCUMENTED UNDER SSCANF
        switch (format.charAt(i + 1)) {
          case 'F':
            // Not supported in PHP sscanf; the argument is treated as a float, and
            //  presented as a floating-point number (non-locale aware)
            // sscanf doesn't support locales, so no need for two (see %f)
            break;
          case 'g':
            // Not supported in PHP sscanf; shorter of %e and %f
            // Irrelevant to input conversion
            break;
          case 'G':
            // Not supported in PHP sscanf; shorter of %E and %f
            // Irrelevant to input conversion
            break;
          case 'b':
            // Not supported in PHP sscanf; the argument is treated as an integer,
            // and presented as a binary number
            // Not supported - couldn't distinguish from other integers
            break;
          case 'i':
            // Integer with base detection (Equivalent of 'd', but base 0 instead of 10)
            var pattern = /([+-])?(?:(?:0x([\da-fA-F]+))|(?:0([0-7]+))|(\d+))/;
            j = _addNext(j, pattern, function (num, sign, hex, oct, dec) {
              return hex ? parseInt(num, 16) : oct ? parseInt(num, 8) : parseInt(num, 10);
            });
            break;
          case 'n':
            // Number of characters processed so far
            retArr[digit !== undefined ? digit : retArr.length - 1] = j;
            break;
          // DOCUMENTED UNDER SPRINTF
          case 'c':
            // Get character; suppresses skipping over whitespace!
            // (but shouldn't be whitespace in format anyways, so no difference here)
            // Non-greedy match
            j = _addNext(j, new RegExp('.{1,' + (width || 1) + '}'));
            break;
          case 'D':
          case 'd':
            // sscanf documented decimal number; equivalent of 'd';
            // Optionally signed decimal integer
            j = _addNext(j, /([+-])?(?:0*)(\d+)/, function (num, sign, dec) {
              // Ignores initial zeroes, unlike %i and parseInt()
              var decInt = parseInt((sign || '') + dec, 10);
              if (decInt < 0) {
                // PHP also won't allow less than -2147483648
                // integer overflow with negative
                return decInt < -2147483648 ? -2147483648 : decInt;
              } else {
                // PHP also won't allow greater than -2147483647
                return decInt < 2147483647 ? decInt : 2147483647;
              }
            });
            break;
          case 'f':
          case 'E':
          case 'e':
            // Although sscanf doesn't support locales,
            // this is used instead of '%F'; seems to be same as %e
            // These don't discriminate here as both allow exponential float of either case
            j = _addNext(j, /([+-])?(?:0*)(\d*\.?\d*(?:[eE]?\d+)?)/, function (num, sign, dec) {
              if (dec === '.') {
                return null;
              }
              // Ignores initial zeroes, unlike %i and parseFloat()
              return parseFloat((sign || '') + dec);
            });
            break;
          case 'u':
            // unsigned decimal integer
            // We won't deal with integer overflows due to signs
            j = _addNext(j, /([+-])?(?:0*)(\d+)/, function (num, sign, dec) {
              // Ignores initial zeroes, unlike %i and parseInt()
              var decInt = parseInt(dec, 10);
              if (sign === '-') {
                // PHP also won't allow greater than 4294967295
                // integer overflow with negative
                return 4294967296 - decInt;
              } else {
                return decInt < 4294967295 ? decInt : 4294967295;
              }
            });
            break;
          case 'o':
            // Octal integer // @todo: add overflows as above?
            j = _addNext(j, /([+-])?(?:0([0-7]+))/, function (num, sign, oct) {
              return parseInt(num, 8);
            });
            break;
          case 's':
            // Greedy match
            j = _addNext(j, /\S+/);
            break;
          case 'X':
          case 'x':
            // Same as 'x'?
            // @todo: add overflows as above?
            // Initial 0x not necessary here
            j = _addNext(j, /([+-])?(?:(?:0x)?([\da-fA-F]+))/, function (num, sign, hex) {
              return parseInt(num, 16);
            });
            break;
          case '':
            // If no character left in expression
            throw new Error('Missing character after percent mark in sscanf() format argument');
          default:
            throw new Error('Unrecognized character after percent mark in sscanf() format argument');
        }
      } catch (e) {
        if (e === 'No match in string') {
          // Allow us to exit
          return _setExtraConversionSpecs(i + 2);
        }
        // Calculate skipping beyond initial percent too
      }
      ++i;
    } else if (format.charAt(i) !== str.charAt(j)) {
      // @todo: Double-check i whitespace ignored in string and/or formats
      _NWS.lastIndex = 0;
      if (_NWS.test(str.charAt(j)) || str.charAt(j) === '') {
        // Whitespace doesn't need to be an exact match)
        return _setExtraConversionSpecs(i + 1);
      } else {
        // Adjust strings when encounter non-matching whitespace,
        // so they align in future checks above
        // Ok to replace with j++;?
        str = str.slice(0, j) + str.slice(j + 1);
        i--;
      }
    } else {
      j++;
    }
  }

  // POST-PROCESSING
  return _finish();
};
//# sourceMappingURL=sscanf.js.map