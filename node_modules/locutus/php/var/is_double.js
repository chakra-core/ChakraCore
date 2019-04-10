'use strict';

module.exports = function is_double(mixedVar) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/is_double/
  // original by: Paulo Freitas
  //      note 1: 1.0 is simplified to 1 before it can be accessed by the function, this makes
  //      note 1: it different from the PHP implementation. We can't fix this unfortunately.
  //   example 1: is_double(186.31)
  //   returns 1: true

  var _isFloat = require('../var/is_float');
  return _isFloat(mixedVar);
};
//# sourceMappingURL=is_double.js.map