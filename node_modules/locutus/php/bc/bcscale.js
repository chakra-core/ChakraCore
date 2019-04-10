'use strict';

module.exports = function bcscale(scale) {
  //  discuss at: http://locutus.io/php/bcscale/
  // original by: lmeyrick (https://sourceforge.net/projects/bcmath-js/)
  //   example 1: bcscale(1)
  //   returns 1: true

  var _bc = require('../_helpers/_bc');
  var libbcmath = _bc();

  scale = parseInt(scale, 10);
  if (isNaN(scale)) {
    return false;
  }
  if (scale < 0) {
    return false;
  }
  libbcmath.scale = scale;

  return true;
};
//# sourceMappingURL=bcscale.js.map