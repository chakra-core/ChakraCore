'use strict';

module.exports = function key(arr) {
  //  discuss at: http://locutus.io/php/key/
  // original by: Brett Zamir (http://brett-zamir.me)
  //    input by: Riddler (http://www.frontierwebdev.com/)
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  //      note 1: Uses global: locutus to store the array pointer
  //   example 1: var $array = {fruit1: 'apple', 'fruit2': 'orange'}
  //   example 1: key($array)
  //   returns 1: 'fruit1'

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
  var cursor = pointers[pointers.indexOf(arr) + 1];
  if (Object.prototype.toString.call(arr) !== '[object Array]') {
    var ct = 0;
    for (var k in arr) {
      if (ct === cursor) {
        return k;
      }
      ct++;
    }
    // Empty
    return false;
  }
  if (arr.length === 0) {
    return false;
  }

  return cursor;
};
//# sourceMappingURL=key.js.map