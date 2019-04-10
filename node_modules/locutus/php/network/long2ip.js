'use strict';

module.exports = function long2ip(ip) {
  //  discuss at: http://locutus.io/php/long2ip/
  // original by: Waldo Malqui Silva (http://waldo.malqui.info)
  //   example 1: long2ip( 3221234342 )
  //   returns 1: '192.0.34.166'

  if (!isFinite(ip)) {
    return false;
  }

  return [ip >>> 24, ip >>> 16 & 0xFF, ip >>> 8 & 0xFF, ip & 0xFF].join('.');
};
//# sourceMappingURL=long2ip.js.map