'use strict';

module.exports = function is_integer(mixedVar) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/is_integer/
  // original by: Paulo Freitas
  //      note 1: 1.0 is simplified to 1 before it can be accessed by the function, this makes
  //      note 1: it different from the PHP implementation. We can't fix this unfortunately.
  //   example 1: is_integer(186.31)
  //   returns 1: false
  //   example 2: is_integer(12)
  //   returns 2: true

  var _isInt = require('../var/is_int');
  return _isInt(mixedVar);
};
//# sourceMappingURL=is_integer.js.map