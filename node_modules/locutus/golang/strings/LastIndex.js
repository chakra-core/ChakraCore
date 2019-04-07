"use strict";

module.exports = function LastIndex(s, sep) {
  //  discuss at: http://locutus.io/golang/strings/LastIndex
  // original by: Kevin van Zonneveld (http://kvz.io)
  //    input by: GopherJS (http://www.gopherjs.org/)
  //   example 1: LastIndex('go gopher', 'go')
  //   returns 1: 3
  //   example 2: LastIndex('go gopher', 'rodent')
  //   returns 2: -1

  return parseInt(s.lastIndexOf(sep), 10) >> 0;
};
//# sourceMappingURL=LastIndex.js.map