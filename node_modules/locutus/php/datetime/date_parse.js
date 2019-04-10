'use strict';

module.exports = function date_parse(date) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/date_parse/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: date_parse('2006-12-12 10:00:00')
  //   returns 1: {year : 2006, month: 12, day: 12, hour: 10, minute: 0, second: 0, fraction: 0, is_localtime: false}

  var strtotime = require('../datetime/strtotime');
  var ts;

  try {
    ts = strtotime(date);
  } catch (e) {
    ts = false;
  }

  if (!ts) {
    return false;
  }

  var dt = new Date(ts * 1000);

  var retObj = {};

  retObj.year = dt.getFullYear();
  retObj.month = dt.getMonth() + 1;
  retObj.day = dt.getDate();
  retObj.hour = dt.getHours();
  retObj.minute = dt.getMinutes();
  retObj.second = dt.getSeconds();
  retObj.fraction = parseFloat('0.' + dt.getMilliseconds());
  retObj.is_localtime = dt.getTimezoneOffset() !== 0;

  return retObj;
};
//# sourceMappingURL=date_parse.js.map