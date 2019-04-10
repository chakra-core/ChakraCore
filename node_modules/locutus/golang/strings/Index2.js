'use strict';

module.exports = function Index(s, sep) {
  //  discuss at: http://locutus.io/golang/strings/Index
  // original by: Kevin van Zonneveld (http://kvz.io)
  //   example 1: Index('Kevin', 'K')
  //   returns 1: 0
  //   example 2: Index('Kevin', 'Z')
  //   returns 2: -1

  return (s + '').indexOf(sep);
};
//# sourceMappingURL=Index2.js.map