'use strict';

module.exports = function gmstrftime(format, timestamp) {
  //  discuss at: http://locutus.io/php/gmstrftime/
  // original by: Brett Zamir (http://brett-zamir.me)
  //    input by: Alex
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  //   example 1: gmstrftime("%A", 1062462400)
  //   returns 1: 'Tuesday'

  var strftime = require('../datetime/strftime');

  var _date = typeof timestamp === 'undefined' ? new Date() : timestamp instanceof Date ? new Date(timestamp) : new Date(timestamp * 1000);

  timestamp = Date.parse(_date.toUTCString().slice(0, -4)) / 1000;

  return strftime(format, timestamp);
};
//# sourceMappingURL=gmstrftime.js.map