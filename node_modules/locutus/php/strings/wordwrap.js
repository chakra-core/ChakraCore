'use strict';

module.exports = function wordwrap(str, intWidth, strBreak, cut) {
  //  discuss at: http://locutus.io/php/wordwrap/
  // original by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
  // improved by: Nick Callen
  // improved by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Sakimori
  //  revised by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
  // bugfixed by: Michael Grier
  // bugfixed by: Feras ALHAEK
  // improved by: Rafał Kukawski (http://kukawski.net)
  //   example 1: wordwrap('Kevin van Zonneveld', 6, '|', true)
  //   returns 1: 'Kevin|van|Zonnev|eld'
  //   example 2: wordwrap('The quick brown fox jumped over the lazy dog.', 20, '<br />\n')
  //   returns 2: 'The quick brown fox<br />\njumped over the lazy<br />\ndog.'
  //   example 3: wordwrap('Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.')
  //   returns 3: 'Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod\ntempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim\nveniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea\ncommodo consequat.'

  intWidth = arguments.length >= 2 ? +intWidth : 75;
  strBreak = arguments.length >= 3 ? '' + strBreak : '\n';
  cut = arguments.length >= 4 ? !!cut : false;

  var i, j, line;

  str += '';

  if (intWidth < 1) {
    return str;
  }

  var reLineBreaks = /\r\n|\n|\r/;
  var reBeginningUntilFirstWhitespace = /^\S*/;
  var reLastCharsWithOptionalTrailingWhitespace = /\S*(\s)?$/;

  var lines = str.split(reLineBreaks);
  var l = lines.length;
  var match;

  // for each line of text
  for (i = 0; i < l; lines[i++] += line) {
    line = lines[i];
    lines[i] = '';

    while (line.length > intWidth) {
      // get slice of length one char above limit
      var slice = line.slice(0, intWidth + 1);

      // remove leading whitespace from rest of line to parse
      var ltrim = 0;
      // remove trailing whitespace from new line content
      var rtrim = 0;

      match = slice.match(reLastCharsWithOptionalTrailingWhitespace);

      // if the slice ends with whitespace
      if (match[1]) {
        // then perfect moment to cut the line
        j = intWidth;
        ltrim = 1;
      } else {
        // otherwise cut at previous whitespace
        j = slice.length - match[0].length;

        if (j) {
          rtrim = 1;
        }

        // but if there is no previous whitespace
        // and cut is forced
        // cut just at the defined limit
        if (!j && cut && intWidth) {
          j = intWidth;
        }

        // if cut wasn't forced
        // cut at next possible whitespace after the limit
        if (!j) {
          var charsUntilNextWhitespace = (line.slice(intWidth).match(reBeginningUntilFirstWhitespace) || [''])[0];

          j = slice.length + charsUntilNextWhitespace.length;
        }
      }

      lines[i] += line.slice(0, j - rtrim);
      line = line.slice(j + ltrim);
      lines[i] += line.length ? strBreak : '';
    }
  }

  return lines.join('\n');
};
//# sourceMappingURL=wordwrap.js.map