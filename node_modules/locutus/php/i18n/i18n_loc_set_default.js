'use strict';

module.exports = function i18n_loc_set_default(name) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/i18n_loc_set_default/
  // original by: Brett Zamir (http://brett-zamir.me)
  //      note 1: Renamed in PHP6 from locale_set_default(). Not listed yet at php.net
  //      note 1: List of locales at http://demo.icu-project.org/icu-bin/locexp (use for implementing other locales here)
  //      note 1: To be usable with sort() if it is passed the SORT_LOCALE_STRING sorting flag: http://php.net/manual/en/function.sort.php
  //   example 1: i18n_loc_set_default('pt_PT')
  //   returns 1: true

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};
  $locutus.php.locales = $locutus.php.locales || {};

  $locutus.php.locales.en_US_POSIX = {
    sorting: function sorting(str1, str2) {
      // @todo: This one taken from strcmp, but need for other locales;
      // we don't use localeCompare since its locale is not settable
      return str1 === str2 ? 0 : str1 > str2 ? 1 : -1;
    }
  };

  $locutus.php.locale_default = name;
  return true;
};
//# sourceMappingURL=i18n_loc_set_default.js.map