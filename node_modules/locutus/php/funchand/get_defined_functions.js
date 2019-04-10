'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function get_defined_functions() {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/get_defined_functions/
  // original by: Brett Zamir (http://brett-zamir.me)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //      note 1: Test case 1: If get_defined_functions can find
  //      note 1: itself in the defined functions, it worked :)
  //   example 1: function test_in_array (array, p_val) {for(var i = 0, l = array.length; i < l; i++) {if (array[i] === p_val) return true} return false}
  //   example 1: var $funcs = get_defined_functions()
  //   example 1: var $found = test_in_array($funcs, 'get_defined_functions')
  //   example 1: var $result = $found
  //   returns 1: true
  //        test: skip-1

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};

  var i = '';
  var arr = [];
  var already = {};

  for (i in $global) {
    try {
      if (typeof $global[i] === 'function') {
        if (!already[i]) {
          already[i] = 1;
          arr.push(i);
        }
      } else if (_typeof($global[i]) === 'object') {
        for (var j in $global[i]) {
          if (typeof $global[j] === 'function' && $global[j] && !already[j]) {
            already[j] = 1;
            arr.push(j);
          }
        }
      }
    } catch (e) {
      // Some objects in Firefox throw exceptions when their
      // properties are accessed (e.g., sessionStorage)
    }
  }

  return arr;
};
//# sourceMappingURL=get_defined_functions.js.map