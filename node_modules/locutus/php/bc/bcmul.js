'use strict';

module.exports = function bcmul(leftOperand, rightOperand, scale) {
  //  discuss at: http://locutus.io/php/bcmul/
  // original by: lmeyrick (https://sourceforge.net/projects/bcmath-js/)
  //   example 1: bcmul('1', '2')
  //   returns 1: '2'
  //   example 2: bcmul('-3', '5')
  //   returns 2: '-15'
  //   example 3: bcmul('1234567890', '9876543210')
  //   returns 3: '12193263111263526900'
  //   example 4: bcmul('2.5', '1.5', 2)
  //   returns 4: '3.75'

  var _bc = require('../_helpers/_bc');
  var libbcmath = _bc();

  var first, second, result;

  if (typeof scale === 'undefined') {
    scale = libbcmath.scale;
  }
  scale = scale < 0 ? 0 : scale;

  // create objects
  first = libbcmath.bc_init_num();
  second = libbcmath.bc_init_num();
  result = libbcmath.bc_init_num();

  first = libbcmath.php_str2num(leftOperand.toString());
  second = libbcmath.php_str2num(rightOperand.toString());

  result = libbcmath.bc_multiply(first, second, scale);

  if (result.n_scale > scale) {
    result.n_scale = scale;
  }
  return result.toString();
};
//# sourceMappingURL=bcmul.js.map