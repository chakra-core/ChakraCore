'use strict';

module.exports = function uksort(inputArr, sorter) {
  //  discuss at: http://locutus.io/php/uksort/
  // original by: Brett Zamir (http://brett-zamir.me)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //      note 1: The examples are correct, this is a new way
  //      note 1: This function deviates from PHP in returning a copy of the array instead
  //      note 1: of acting by reference and returning true; this was necessary because
  //      note 1: IE does not allow deleting and re-adding of properties without caching
  //      note 1: of property position; you can set the ini of "locutus.sortByReference" to true to
  //      note 1: get the PHP behavior, but use this only if you are in an environment
  //      note 1: such as Firefox extensions where for-in iteration order is fixed and true
  //      note 1: property deletion is supported. Note that we intend to implement the PHP
  //      note 1: behavior by default if IE ever does allow it; only gives shallow copy since
  //      note 1: is by reference in PHP anyways
  //   example 1: var $data = {d: 'lemon', a: 'orange', b: 'banana', c: 'apple'}
  //   example 1: uksort($data, function (key1, key2){ return (key1 === key2 ? 0 : (key1 > key2 ? 1 : -1)); })
  //   example 1: var $result = $data
  //   returns 1: {a: 'orange', b: 'banana', c: 'apple', d: 'lemon'}

  var tmpArr = {};
  var keys = [];
  var i = 0;
  var k = '';
  var sortByReference = false;
  var populateArr = {};

  if (typeof sorter === 'string') {
    sorter = this.window[sorter];
  }

  // Make a list of key names
  for (k in inputArr) {
    if (inputArr.hasOwnProperty(k)) {
      keys.push(k);
    }
  }

  // Sort key names
  try {
    if (sorter) {
      keys.sort(sorter);
    } else {
      keys.sort();
    }
  } catch (e) {
    return false;
  }

  var iniVal = (typeof require !== 'undefined' ? require('../info/ini_get')('locutus.sortByReference') : undefined) || 'on';
  sortByReference = iniVal === 'on';
  populateArr = sortByReference ? inputArr : populateArr;

  // Rebuild array with sorted key names
  for (i = 0; i < keys.length; i++) {
    k = keys[i];
    tmpArr[k] = inputArr[k];
    if (sortByReference) {
      delete inputArr[k];
    }
  }
  for (i in tmpArr) {
    if (tmpArr.hasOwnProperty(i)) {
      populateArr[i] = tmpArr[i];
    }
  }

  return sortByReference || populateArr;
};
//# sourceMappingURL=uksort.js.map