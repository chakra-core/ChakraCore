'use strict';

module.exports = function localeconv() {
  //  discuss at: http://locutus.io/php/localeconv/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: setlocale('LC_ALL', 'en_US')
  //   example 1: localeconv()
  //   returns 1: {decimal_point: '.', thousands_sep: '', positive_sign: '', negative_sign: '-', int_frac_digits: 2, frac_digits: 2, p_cs_precedes: 1, p_sep_by_space: 0, n_cs_precedes: 1, n_sep_by_space: 0, p_sign_posn: 1, n_sign_posn: 1, grouping: [], int_curr_symbol: 'USD ', currency_symbol: '$', mon_decimal_point: '.', mon_thousands_sep: ',', mon_grouping: [3, 3]}

  var setlocale = require('../strings/setlocale');

  var arr = {};
  var prop = '';

  // ensure setup of localization variables takes place, if not already
  setlocale('LC_ALL', 0);

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};

  // Make copies
  for (prop in $locutus.php.locales[$locutus.php.localeCategories.LC_NUMERIC].LC_NUMERIC) {
    arr[prop] = $locutus.php.locales[$locutus.php.localeCategories.LC_NUMERIC].LC_NUMERIC[prop];
  }
  for (prop in $locutus.php.locales[$locutus.php.localeCategories.LC_MONETARY].LC_MONETARY) {
    arr[prop] = $locutus.php.locales[$locutus.php.localeCategories.LC_MONETARY].LC_MONETARY[prop];
  }

  return arr;
};
//# sourceMappingURL=localeconv.js.map