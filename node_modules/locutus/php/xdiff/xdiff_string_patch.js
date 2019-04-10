'use strict';

module.exports = function xdiff_string_patch(originalStr, patch, flags, errorObj) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/xdiff_string_patch/
  // original by: Brett Zamir (http://brett-zamir.me)
  // improved by: Steven Levithan (stevenlevithan.com)
  //      note 1: The XDIFF_PATCH_IGNORESPACE flag and the error argument are not
  //      note 1: currently supported.
  //      note 2: This has not been tested exhaustively yet.
  //      note 3: The errorObj parameter (optional) if used must be passed in as a
  //      note 3: object. The errors will then be written by reference into it's `value` property
  //   example 1: xdiff_string_patch('', '@@ -0,0 +1,1 @@\n+Hello world!')
  //   returns 1: 'Hello world!'

  // First two functions were adapted from Steven Levithan, also under an MIT license
  // Adapted from XRegExp 1.5.0
  // (c) 2007-2010 Steven Levithan
  // MIT License
  // <http://xregexp.com>

  var _getNativeFlags = function _getNativeFlags(regex) {
    // Proposed for ES4; included in AS3
    return [regex.global ? 'g' : '', regex.ignoreCase ? 'i' : '', regex.multiline ? 'm' : '', regex.extended ? 'x' : '', regex.sticky ? 'y' : ''].join('');
  };

  var _cbSplit = function _cbSplit(string, sep) {
    // If separator `s` is not a regex, use the native `split`
    if (!(sep instanceof RegExp)) {
      // Had problems to get it to work here using prototype test
      return String.prototype.split.apply(string, arguments);
    }
    var str = String(string);
    var output = [];
    var lastLastIndex = 0;
    var match;
    var lastLength;
    var limit = Infinity;
    var x = sep._xregexp;
    // This is required if not `s.global`, and it avoids needing to set `s.lastIndex` to zero
    // and restore it to its original value when we're done using the regex
    // Brett paring down
    var s = new RegExp(sep.source, _getNativeFlags(sep) + 'g');
    if (x) {
      s._xregexp = {
        source: x.source,
        captureNames: x.captureNames ? x.captureNames.slice(0) : null
      };
    }

    while (match = s.exec(str)) {
      // Run the altered `exec` (required for `lastIndex` fix, etc.)
      if (s.lastIndex > lastLastIndex) {
        output.push(str.slice(lastLastIndex, match.index));

        if (match.length > 1 && match.index < str.length) {
          Array.prototype.push.apply(output, match.slice(1));
        }

        lastLength = match[0].length;
        lastLastIndex = s.lastIndex;

        if (output.length >= limit) {
          break;
        }
      }

      if (s.lastIndex === match.index) {
        s.lastIndex++;
      }
    }

    if (lastLastIndex === str.length) {
      if (!s.test('') || lastLength) {
        output.push('');
      }
    } else {
      output.push(str.slice(lastLastIndex));
    }

    return output.length > limit ? output.slice(0, limit) : output;
  };

  var i = 0;
  var ll = 0;
  var ranges = [];
  var lastLinePos = 0;
  var firstChar = '';
  var rangeExp = /^@@\s+-(\d+),(\d+)\s+\+(\d+),(\d+)\s+@@$/;
  var lineBreaks = /\r?\n/;
  var lines = _cbSplit(patch.replace(/(\r?\n)+$/, ''), lineBreaks);
  var origLines = _cbSplit(originalStr, lineBreaks);
  var newStrArr = [];
  var linePos = 0;
  var errors = '';
  var optTemp = 0; // Both string & integer (constant) input is allowed
  var OPTS = {
    // Unsure of actual PHP values, so better to rely on string
    'XDIFF_PATCH_NORMAL': 1,
    'XDIFF_PATCH_REVERSE': 2,
    'XDIFF_PATCH_IGNORESPACE': 4
  };

  // Input defaulting & sanitation
  if (typeof originalStr !== 'string' || !patch) {
    return false;
  }
  if (!flags) {
    flags = 'XDIFF_PATCH_NORMAL';
  }

  if (typeof flags !== 'number') {
    // Allow for a single string or an array of string flags
    flags = [].concat(flags);
    for (i = 0; i < flags.length; i++) {
      // Resolve string input to bitwise e.g. 'XDIFF_PATCH_NORMAL' becomes 1
      if (OPTS[flags[i]]) {
        optTemp = optTemp | OPTS[flags[i]];
      }
    }
    flags = optTemp;
  }

  if (flags & OPTS.XDIFF_PATCH_NORMAL) {
    for (i = 0, ll = lines.length; i < ll; i++) {
      ranges = lines[i].match(rangeExp);
      if (ranges) {
        lastLinePos = linePos;
        linePos = ranges[1] - 1;
        while (lastLinePos < linePos) {
          newStrArr[newStrArr.length] = origLines[lastLinePos++];
        }
        while (lines[++i] && rangeExp.exec(lines[i]) === null) {
          firstChar = lines[i].charAt(0);
          switch (firstChar) {
            case '-':
              // Skip including that line
              ++linePos;
              break;
            case '+':
              newStrArr[newStrArr.length] = lines[i].slice(1);
              break;
            case ' ':
              newStrArr[newStrArr.length] = origLines[linePos++];
              break;
            default:
              // Reconcile with returning errrors arg?
              throw new Error('Unrecognized initial character in unidiff line');
          }
        }
        if (lines[i]) {
          i--;
        }
      }
    }
    while (linePos > 0 && linePos < origLines.length) {
      newStrArr[newStrArr.length] = origLines[linePos++];
    }
  } else if (flags & OPTS.XDIFF_PATCH_REVERSE) {
    // Only differs from above by a few lines
    for (i = 0, ll = lines.length; i < ll; i++) {
      ranges = lines[i].match(rangeExp);
      if (ranges) {
        lastLinePos = linePos;
        linePos = ranges[3] - 1;
        while (lastLinePos < linePos) {
          newStrArr[newStrArr.length] = origLines[lastLinePos++];
        }
        while (lines[++i] && rangeExp.exec(lines[i]) === null) {
          firstChar = lines[i].charAt(0);
          switch (firstChar) {
            case '-':
              newStrArr[newStrArr.length] = lines[i].slice(1);
              break;
            case '+':
              // Skip including that line
              ++linePos;
              break;
            case ' ':
              newStrArr[newStrArr.length] = origLines[linePos++];
              break;
            default:
              // Reconcile with returning errrors arg?
              throw new Error('Unrecognized initial character in unidiff line');
          }
        }
        if (lines[i]) {
          i--;
        }
      }
    }
    while (linePos > 0 && linePos < origLines.length) {
      newStrArr[newStrArr.length] = origLines[linePos++];
    }
  }

  if (errorObj) {
    errorObj.value = errors;
  }

  return newStrArr.join('\n');
};
//# sourceMappingURL=xdiff_string_patch.js.map