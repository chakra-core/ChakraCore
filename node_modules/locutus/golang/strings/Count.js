'use strict';

module.exports = function Count(s, sep) {
  //  discuss at: http://locutus.io/php/printf/
  // original by: Kevin van Zonneveld (http://kvz.io)
  //    input by: GopherJS (http://www.gopherjs.org/)
  //   example 1: Count("cheese", "e")
  //   returns 1: 3
  //   example 2: Count("five", "") // before & after each rune
  //   returns 2: 5

  var pos;
  var n = 0;

  if (sep.length === 0) {
    return s.split(sep).length + 1;
  } else if (sep.length > s.length) {
    return 0;
  } else if (sep.length === s.length) {
    if (sep === s) {
      return 1;
    }
    return 0;
  }
  while (true) {
    pos = (s + '').indexOf(sep);
    if (pos === -1) {
      break;
    }
    n = n + 1 >> 0;
    s = s.substring(pos + sep.length >> 0);
  }
  return n;
};
//# sourceMappingURL=Count.js.map