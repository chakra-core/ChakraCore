'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function array_multisort(arr) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_multisort/
  // original by: Theriault (https://github.com/Theriault)
  // improved by: Oleg Andreyev (https://github.com/oleg-andreyev)
  //   example 1: array_multisort([1, 2, 1, 2, 1, 2], [1, 2, 3, 4, 5, 6])
  //   returns 1: true
  //   example 2: var $characters = {A: 'Edward', B: 'Locke', C: 'Sabin', D: 'Terra', E: 'Edward'}
  //   example 2: var $jobs = {A: 'Warrior', B: 'Thief', C: 'Monk', D: 'Mage', E: 'Knight'}
  //   example 2: array_multisort($characters, 'SORT_DESC', 'SORT_STRING', $jobs, 'SORT_ASC', 'SORT_STRING')
  //   returns 2: true
  //   example 3: var $lastnames = [ 'Carter','Adams','Monroe','Tyler','Madison','Kennedy','Adams']
  //   example 3: var $firstnames = ['James', 'John' ,'James', 'John', 'James',  'John',   'John']
  //   example 3: var $president = [ 39, 6, 5, 10, 4, 35, 2 ]
  //   example 3: array_multisort($firstnames, 'SORT_DESC', 'SORT_STRING', $lastnames, 'SORT_ASC', 'SORT_STRING', $president, 'SORT_NUMERIC')
  //   returns 3: true
  //      note 1: flags: Translation table for sort arguments.
  //      note 1: Each argument turns on certain bits in the flag byte through addition.
  //      note 1: bits: HGFE DCBA
  //      note 1: args: Holds pointer to arguments for reassignment

  var g;
  var i;
  var j;
  var k;
  var l;
  var sal;
  var vkey;
  var elIndex;
  var lastSorts;
  var tmpArray;
  var zlast;

  var sortFlag = [0];
  var thingsToSort = [];
  var nLastSort = [];
  var lastSort = [];
  // possibly redundant
  var args = arguments;

  var flags = {
    'SORT_REGULAR': 16,
    'SORT_NUMERIC': 17,
    'SORT_STRING': 18,
    'SORT_ASC': 32,
    'SORT_DESC': 40
  };

  var sortDuplicator = function sortDuplicator(a, b) {
    return nLastSort.shift();
  };

  var sortFunctions = [[function (a, b) {
    lastSort.push(a > b ? 1 : a < b ? -1 : 0);
    return a > b ? 1 : a < b ? -1 : 0;
  }, function (a, b) {
    lastSort.push(b > a ? 1 : b < a ? -1 : 0);
    return b > a ? 1 : b < a ? -1 : 0;
  }], [function (a, b) {
    lastSort.push(a - b);
    return a - b;
  }, function (a, b) {
    lastSort.push(b - a);
    return b - a;
  }], [function (a, b) {
    lastSort.push(a + '' > b + '' ? 1 : a + '' < b + '' ? -1 : 0);
    return a + '' > b + '' ? 1 : a + '' < b + '' ? -1 : 0;
  }, function (a, b) {
    lastSort.push(b + '' > a + '' ? 1 : b + '' < a + '' ? -1 : 0);
    return b + '' > a + '' ? 1 : b + '' < a + '' ? -1 : 0;
  }]];

  var sortArrs = [[]];

  var sortKeys = [[]];

  // Store first argument into sortArrs and sortKeys if an Object.
  // First Argument should be either a Javascript Array or an Object,
  // otherwise function would return FALSE like in PHP
  if (Object.prototype.toString.call(arr) === '[object Array]') {
    sortArrs[0] = arr;
  } else if (arr && (typeof arr === 'undefined' ? 'undefined' : _typeof(arr)) === 'object') {
    for (i in arr) {
      if (arr.hasOwnProperty(i)) {
        sortKeys[0].push(i);
        sortArrs[0].push(arr[i]);
      }
    }
  } else {
    return false;
  }

  // arrMainLength: Holds the length of the first array.
  // All other arrays must be of equal length, otherwise function would return FALSE like in PHP
  // sortComponents: Holds 2 indexes per every section of the array
  // that can be sorted. As this is the start, the whole array can be sorted.
  var arrMainLength = sortArrs[0].length;
  var sortComponents = [0, arrMainLength];

  // Loop through all other arguments, checking lengths and sort flags
  // of arrays and adding them to the above variables.
  var argl = arguments.length;
  for (j = 1; j < argl; j++) {
    if (Object.prototype.toString.call(arguments[j]) === '[object Array]') {
      sortArrs[j] = arguments[j];
      sortFlag[j] = 0;
      if (arguments[j].length !== arrMainLength) {
        return false;
      }
    } else if (arguments[j] && _typeof(arguments[j]) === 'object') {
      sortKeys[j] = [];
      sortArrs[j] = [];
      sortFlag[j] = 0;
      for (i in arguments[j]) {
        if (arguments[j].hasOwnProperty(i)) {
          sortKeys[j].push(i);
          sortArrs[j].push(arguments[j][i]);
        }
      }
      if (sortArrs[j].length !== arrMainLength) {
        return false;
      }
    } else if (typeof arguments[j] === 'string') {
      var lFlag = sortFlag.pop();
      // Keep extra parentheses around latter flags check
      // to avoid minimization leading to CDATA closer
      if (typeof flags[arguments[j]] === 'undefined' || (flags[arguments[j]] >>> 4 & lFlag >>> 4) > 0) {
        return false;
      }
      sortFlag.push(lFlag + flags[arguments[j]]);
    } else {
      return false;
    }
  }

  for (i = 0; i !== arrMainLength; i++) {
    thingsToSort.push(true);
  }

  // Sort all the arrays....
  for (i in sortArrs) {
    if (sortArrs.hasOwnProperty(i)) {
      lastSorts = [];
      tmpArray = [];
      elIndex = 0;
      nLastSort = [];
      lastSort = [];

      // If there are no sortComponents, then no more sorting is neeeded.
      // Copy the array back to the argument.
      if (sortComponents.length === 0) {
        if (Object.prototype.toString.call(arguments[i]) === '[object Array]') {
          args[i] = sortArrs[i];
        } else {
          for (k in arguments[i]) {
            if (arguments[i].hasOwnProperty(k)) {
              delete arguments[i][k];
            }
          }
          sal = sortArrs[i].length;
          for (j = 0, vkey = 0; j < sal; j++) {
            vkey = sortKeys[i][j];
            args[i][vkey] = sortArrs[i][j];
          }
        }
        sortArrs.splice(i, 1);
        sortKeys.splice(i, 1);
        continue;
      }

      // Sort function for sorting. Either sorts asc or desc, regular/string or numeric.
      var sFunction = sortFunctions[sortFlag[i] & 3][(sortFlag[i] & 8) > 0 ? 1 : 0];

      // Sort current array.
      for (l = 0; l !== sortComponents.length; l += 2) {
        tmpArray = sortArrs[i].slice(sortComponents[l], sortComponents[l + 1] + 1);
        tmpArray.sort(sFunction);
        // Is there a better way to copy an array in Javascript?
        lastSorts[l] = [].concat(lastSort);
        elIndex = sortComponents[l];
        for (g in tmpArray) {
          if (tmpArray.hasOwnProperty(g)) {
            sortArrs[i][elIndex] = tmpArray[g];
            elIndex++;
          }
        }
      }

      // Duplicate the sorting of the current array on future arrays.
      sFunction = sortDuplicator;
      for (j in sortArrs) {
        if (sortArrs.hasOwnProperty(j)) {
          if (sortArrs[j] === sortArrs[i]) {
            continue;
          }
          for (l = 0; l !== sortComponents.length; l += 2) {
            tmpArray = sortArrs[j].slice(sortComponents[l], sortComponents[l + 1] + 1);
            // alert(l + ':' + nLastSort);
            nLastSort = [].concat(lastSorts[l]);
            tmpArray.sort(sFunction);
            elIndex = sortComponents[l];
            for (g in tmpArray) {
              if (tmpArray.hasOwnProperty(g)) {
                sortArrs[j][elIndex] = tmpArray[g];
                elIndex++;
              }
            }
          }
        }
      }

      // Duplicate the sorting of the current array on array keys
      for (j in sortKeys) {
        if (sortKeys.hasOwnProperty(j)) {
          for (l = 0; l !== sortComponents.length; l += 2) {
            tmpArray = sortKeys[j].slice(sortComponents[l], sortComponents[l + 1] + 1);
            nLastSort = [].concat(lastSorts[l]);
            tmpArray.sort(sFunction);
            elIndex = sortComponents[l];
            for (g in tmpArray) {
              if (tmpArray.hasOwnProperty(g)) {
                sortKeys[j][elIndex] = tmpArray[g];
                elIndex++;
              }
            }
          }
        }
      }

      // Generate the next sortComponents
      zlast = null;
      sortComponents = [];
      for (j in sortArrs[i]) {
        if (sortArrs[i].hasOwnProperty(j)) {
          if (!thingsToSort[j]) {
            if (sortComponents.length & 1) {
              sortComponents.push(j - 1);
            }
            zlast = null;
            continue;
          }
          if (!(sortComponents.length & 1)) {
            if (zlast !== null) {
              if (sortArrs[i][j] === zlast) {
                sortComponents.push(j - 1);
              } else {
                thingsToSort[j] = false;
              }
            }
            zlast = sortArrs[i][j];
          } else {
            if (sortArrs[i][j] !== zlast) {
              sortComponents.push(j - 1);
              zlast = sortArrs[i][j];
            }
          }
        }
      }

      if (sortComponents.length & 1) {
        sortComponents.push(j);
      }
      if (Object.prototype.toString.call(arguments[i]) === '[object Array]') {
        args[i] = sortArrs[i];
      } else {
        for (j in arguments[i]) {
          if (arguments[i].hasOwnProperty(j)) {
            delete arguments[i][j];
          }
        }

        sal = sortArrs[i].length;
        for (j = 0, vkey = 0; j < sal; j++) {
          vkey = sortKeys[i][j];
          args[i][vkey] = sortArrs[i][j];
        }
      }
      sortArrs.splice(i, 1);
      sortKeys.splice(i, 1);
    }
  }
  return true;
};
//# sourceMappingURL=array_multisort.js.map