"use strict";

module.exports = function gettimeofday(returnFloat) {
  //  discuss at: http://locutus.io/php/gettimeofday/
  // original by: Brett Zamir (http://brett-zamir.me)
  // original by: Josh Fraser (http://onlineaspect.com/2007/06/08/auto-detect-a-time-zone-with-javascript/)
  //    parts by: Breaking Par Consulting Inc (http://www.breakingpar.com/bkp/home.nsf/0/87256B280015193F87256CFB006C45F7)
  //  revised by: Theriault (https://github.com/Theriault)
  //   example 1: var $obj = gettimeofday()
  //   example 1: var $result = ('sec' in $obj && 'usec' in $obj && 'minuteswest' in $obj &&80, 'dsttime' in $obj)
  //   returns 1: true
  //   example 2: var $timeStamp = gettimeofday(true)
  //   example 2: var $result = $timeStamp > 1000000000 && $timeStamp < 2000000000
  //   returns 2: true

  var t = new Date();
  var y = 0;

  if (returnFloat) {
    return t.getTime() / 1000;
  }

  // Store current year.
  y = t.getFullYear();
  return {
    sec: t.getUTCSeconds(),
    usec: t.getUTCMilliseconds() * 1000,
    minuteswest: t.getTimezoneOffset(),
    // Compare Jan 1 minus Jan 1 UTC to Jul 1 minus Jul 1 UTC to see if DST is observed.
    dsttime: 0 + (new Date(y, 0) - Date.UTC(y, 0) !== new Date(y, 6) - Date.UTC(y, 6))
  };
};
//# sourceMappingURL=gettimeofday.js.map