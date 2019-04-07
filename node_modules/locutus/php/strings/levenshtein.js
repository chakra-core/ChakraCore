'use strict';

module.exports = function levenshtein(s1, s2, costIns, costRep, costDel) {
  //       discuss at: http://locutus.io/php/levenshtein/
  //      original by: Carlos R. L. Rodrigues (http://www.jsfromhell.com)
  //      bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  //       revised by: Andrea Giammarchi (http://webreflection.blogspot.com)
  // reimplemented by: Brett Zamir (http://brett-zamir.me)
  // reimplemented by: Alexander M Beedie
  // reimplemented by: Rafał Kukawski (http://blog.kukawski.pl)
  //        example 1: levenshtein('Kevin van Zonneveld', 'Kevin van Sommeveld')
  //        returns 1: 3
  //        example 2: levenshtein("carrrot", "carrots")
  //        returns 2: 2
  //        example 3: levenshtein("carrrot", "carrots", 2, 3, 4)
  //        returns 3: 6

  // var LEVENSHTEIN_MAX_LENGTH = 255 // PHP limits the function to max 255 character-long strings

  costIns = costIns == null ? 1 : +costIns;
  costRep = costRep == null ? 1 : +costRep;
  costDel = costDel == null ? 1 : +costDel;

  if (s1 === s2) {
    return 0;
  }

  var l1 = s1.length;
  var l2 = s2.length;

  if (l1 === 0) {
    return l2 * costIns;
  }
  if (l2 === 0) {
    return l1 * costDel;
  }

  // Enable the 3 lines below to set the same limits on string length as PHP does
  // if (l1 > LEVENSHTEIN_MAX_LENGTH || l2 > LEVENSHTEIN_MAX_LENGTH) {
  //   return -1;
  // }

  var split = false;
  try {
    split = !'0'[0];
  } catch (e) {
    // Earlier IE may not support access by string index
    split = true;
  }

  if (split) {
    s1 = s1.split('');
    s2 = s2.split('');
  }

  var p1 = new Array(l2 + 1);
  var p2 = new Array(l2 + 1);

  var i1, i2, c0, c1, c2, tmp;

  for (i2 = 0; i2 <= l2; i2++) {
    p1[i2] = i2 * costIns;
  }

  for (i1 = 0; i1 < l1; i1++) {
    p2[0] = p1[0] + costDel;

    for (i2 = 0; i2 < l2; i2++) {
      c0 = p1[i2] + (s1[i1] === s2[i2] ? 0 : costRep);
      c1 = p1[i2 + 1] + costDel;

      if (c1 < c0) {
        c0 = c1;
      }

      c2 = p2[i2] + costIns;

      if (c2 < c0) {
        c0 = c2;
      }

      p2[i2 + 1] = c0;
    }

    tmp = p1;
    p1 = p2;
    p2 = tmp;
  }

  c0 = p1[l2];

  return c0;
};
//# sourceMappingURL=levenshtein.js.map