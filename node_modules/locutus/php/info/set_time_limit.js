'use strict';

module.exports = function set_time_limit(seconds) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/set_time_limit/
  // original by: Brett Zamir (http://brett-zamir.me)
  //        test: skip-all
  //   example 1: set_time_limit(4)
  //   returns 1: undefined

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};

  setTimeout(function () {
    if (!$locutus.php.timeoutStatus) {
      $locutus.php.timeoutStatus = true;
    }
    throw new Error('Maximum execution time exceeded');
  }, seconds * 1000);
};
//# sourceMappingURL=set_time_limit.js.map