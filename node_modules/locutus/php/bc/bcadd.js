'use strict';

module.exports = function bcadd(leftOperand, rightOperand, scale) {
  //  discuss at: http://locutus.io/php/bcadd/
  // original by: lmeyrick (https://sourceforge.net/projects/bcmath-js/)
  //   example 1: bcadd('1', '2')
  //   returns 1: '3'
  //   example 2: bcadd('-1', '5', 4)
  //   returns 2: '4.0000'
  //   example 3: bcadd('1928372132132819737213', '8728932001983192837219398127471', 2)
  //   returns 3: '8728932003911564969352217864684.00'

  var bc = require('../_helpers/_bc');
  var libbcmath = bc();

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

  result = libbcmath.bc_add(first, second, scale);

  if (result.n_scale > scale) {
    result.n_scale = scale;
  }

  return result.toString();
};
//# sourceMappingURL=bcadd.js.map