'use strict';

module.exports = function current(arr) {
  //  discuss at: http://locutus.io/php/current/
  // original by: Brett Zamir (http://brett-zamir.me)
  //      note 1: Uses global: locutus to store the array pointer
  //   example 1: var $transport = ['foot', 'bike', 'car', 'plane']
  //   example 1: current($transport)
  //   returns 1: 'foot'

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};
  $locutus.php.pointers = $locutus.php.pointers || [];
  var pointers = $locutus.php.pointers;

  var indexOf = function indexOf(value) {
    for (var i = 0, length = this.length; i < length; i++) {
      if (this[i] === value) {
        return i;
      }
    }
    return -1;
  };
  if (!pointers.indexOf) {
    pointers.indexOf = indexOf;
  }
  if (pointers.indexOf(arr) === -1) {
    pointers.push(arr, 0);
  }
  var arrpos = pointers.indexOf(arr);
  var cursor = pointers[arrpos + 1];
  if (Object.prototype.toString.call(arr) === '[object Array]') {
    return arr[cursor] || false;
  }
  var ct = 0;
  for (var k in arr) {
    if (ct === cursor) {
      return arr[k];
    }
    ct++;
  }
  // Empty
  return false;
};
//# sourceMappingURL=current.js.map