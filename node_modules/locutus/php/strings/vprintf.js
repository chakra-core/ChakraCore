'use strict';

module.exports = function vprintf(format, args) {
  //       discuss at: http://locutus.io/php/vprintf/
  //      original by: Ash Searle (http://hexmen.com/blog/)
  //      improved by: Michael White (http://getsprink.com)
  // reimplemented by: Brett Zamir (http://brett-zamir.me)
  //        example 1: vprintf("%01.2f", 123.1)
  //        returns 1: 6

  var sprintf = require('../strings/sprintf');
  var echo = require('../strings/echo');
  var ret = sprintf.apply(this, [format].concat(args));
  echo(ret);

  return ret.length;
};
//# sourceMappingURL=vprintf.js.map