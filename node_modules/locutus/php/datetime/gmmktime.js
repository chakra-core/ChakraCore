'use strict';

module.exports = function gmmktime() {
  //  discuss at: http://locutus.io/php/gmmktime/
  // original by: Brett Zamir (http://brett-zamir.me)
  // original by: mktime
  //   example 1: gmmktime(14, 10, 2, 2, 1, 2008)
  //   returns 1: 1201875002
  //   example 2: gmmktime(0, 0, -1, 1, 1, 1970)
  //   returns 2: -1

  var d = new Date();
  var r = arguments;
  var i = 0;
  var e = ['Hours', 'Minutes', 'Seconds', 'Month', 'Date', 'FullYear'];

  for (i = 0; i < e.length; i++) {
    if (typeof r[i] === 'undefined') {
      r[i] = d['getUTC' + e[i]]();
      // +1 to fix JS months.
      r[i] += i === 3;
    } else {
      r[i] = parseInt(r[i], 10);
      if (isNaN(r[i])) {
        return false;
      }
    }
  }

  // Map years 0-69 to 2000-2069 and years 70-100 to 1970-2000.
  r[5] += r[5] >= 0 ? r[5] <= 69 ? 2e3 : r[5] <= 100 ? 1900 : 0 : 0;

  // Set year, month (-1 to fix JS months), and date.
  // !This must come before the call to setHours!
  d.setUTCFullYear(r[5], r[3] - 1, r[4]);

  // Set hours, minutes, and seconds.
  d.setUTCHours(r[0], r[1], r[2]);

  var time = d.getTime();

  // Divide milliseconds by 1000 to return seconds and drop decimal.
  // Add 1 second if negative or it'll be off from PHP by 1 second.
  return (time / 1e3 >> 0) - (time < 0);
};
//# sourceMappingURL=gmmktime.js.map