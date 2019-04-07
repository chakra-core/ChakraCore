'use strict';

module.exports = function inet_ntop(a) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/inet_ntop/
  // original by: Theriault (https://github.com/Theriault)
  //   example 1: inet_ntop('\x7F\x00\x00\x01')
  //   returns 1: '127.0.0.1'
  //   _example 2: inet_ntop('\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1')
  //   _returns 2: '::1'

  var i = 0;
  var m = '';
  var c = [];

  a += '';
  if (a.length === 4) {
    // IPv4
    return [a.charCodeAt(0), a.charCodeAt(1), a.charCodeAt(2), a.charCodeAt(3)].join('.');
  } else if (a.length === 16) {
    // IPv6
    for (i = 0; i < 16; i++) {
      c.push(((a.charCodeAt(i++) << 8) + a.charCodeAt(i)).toString(16));
    }
    return c.join(':').replace(/((^|:)0(?=:|$))+:?/g, function (t) {
      m = t.length > m.length ? t : m;
      return t;
    }).replace(m || ' ', '::');
  } else {
    // Invalid length
    return false;
  }
};
//# sourceMappingURL=inet_ntop.js.map