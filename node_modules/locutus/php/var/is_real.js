'use strict';

module.exports = function is_real(mixedVar) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/is_real/
  // original by: Brett Zamir (http://brett-zamir.me)
  //      note 1: 1.0 is simplified to 1 before it can be accessed by the function, this makes
  //      note 1: it different from the PHP implementation. We can't fix this unfortunately.
  //   example 1: is_real(186.31)
  //   returns 1: true

  var _isFloat = require('../var/is_float');
  return _isFloat(mixedVar);
};
//# sourceMappingURL=is_real.js.map