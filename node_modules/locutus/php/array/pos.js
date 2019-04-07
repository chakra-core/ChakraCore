'use strict';

module.exports = function pos(arr) {
  //  discuss at: http://locutus.io/php/pos/
  // original by: Brett Zamir (http://brett-zamir.me)
  //      note 1: Uses global: locutus to store the array pointer
  //   example 1: var $transport = ['foot', 'bike', 'car', 'plane']
  //   example 1: pos($transport)
  //   returns 1: 'foot'

  var current = require('../array/current');
  return current(arr);
};
//# sourceMappingURL=pos.js.map