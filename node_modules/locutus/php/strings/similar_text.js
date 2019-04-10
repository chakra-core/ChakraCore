'use strict';

module.exports = function similar_text(first, second, percent) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/similar_text/
  // original by: Rafał Kukawski (http://blog.kukawski.pl)
  // bugfixed by: Chris McMacken
  // bugfixed by: Jarkko Rantavuori original by findings in stackoverflow (http://stackoverflow.com/questions/14136349/how-does-similar-text-work)
  // improved by: Markus Padourek (taken from http://www.kevinhq.com/2012/06/php-similartext-function-in-javascript_16.html)
  //   example 1: similar_text('Hello World!', 'Hello locutus!')
  //   returns 1: 8
  //   example 2: similar_text('Hello World!', null)
  //   returns 2: 0

  if (first === null || second === null || typeof first === 'undefined' || typeof second === 'undefined') {
    return 0;
  }

  first += '';
  second += '';

  var pos1 = 0;
  var pos2 = 0;
  var max = 0;
  var firstLength = first.length;
  var secondLength = second.length;
  var p;
  var q;
  var l;
  var sum;

  for (p = 0; p < firstLength; p++) {
    for (q = 0; q < secondLength; q++) {
      for (l = 0; p + l < firstLength && q + l < secondLength && first.charAt(p + l) === second.charAt(q + l); l++) {// eslint-disable-line max-len
        // @todo: ^-- break up this crazy for loop and put the logic in its body
      }
      if (l > max) {
        max = l;
        pos1 = p;
        pos2 = q;
      }
    }
  }

  sum = max;

  if (sum) {
    if (pos1 && pos2) {
      sum += similar_text(first.substr(0, pos1), second.substr(0, pos2));
    }

    if (pos1 + max < firstLength && pos2 + max < secondLength) {
      sum += similar_text(first.substr(pos1 + max, firstLength - pos1 - max), second.substr(pos2 + max, secondLength - pos2 - max));
    }
  }

  if (!percent) {
    return sum;
  }

  return sum * 200 / (firstLength + secondLength);
};
//# sourceMappingURL=similar_text.js.map