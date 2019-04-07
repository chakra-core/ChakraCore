'use strict';

module.exports = function printf() {
  //  discuss at: http://locutus.io/php/printf/
  // original by: Ash Searle (http://hexmen.com/blog/)
  // improved by: Michael White (http://getsprink.com)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //   example 1: printf("%01.2f", 123.1)
  //   returns 1: 6

  var sprintf = require('../strings/sprintf');
  var echo = require('../strings/echo');
  var ret = sprintf.apply(this, arguments);
  echo(ret);
  return ret.length;
};
//# sourceMappingURL=printf.js.map