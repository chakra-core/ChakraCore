'use strict';

module.exports = function i18n_loc_get_default() {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/i18n_loc_get_default/
  // original by: Brett Zamir (http://brett-zamir.me)
  //      note 1: Renamed in PHP6 from locale_get_default(). Not listed yet at php.net.
  //      note 1: List of locales at <http://demo.icu-project.org/icu-bin/locexp>
  //      note 1: To be usable with sort() if it is passed the `SORT_LOCALE_STRING`
  //      note 1: sorting flag: http://php.net/manual/en/function.sort.php
  //   example 1: i18n_loc_get_default()
  //   returns 1: 'en_US_POSIX'
  //   example 2: i18n_loc_set_default('pt_PT')
  //   example 2: i18n_loc_get_default()
  //   returns 2: 'pt_PT'

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};
  $locutus.php.locales = $locutus.php.locales || {};

  return $locutus.php.locale_default || 'en_US_POSIX';
};
//# sourceMappingURL=i18n_loc_get_default.js.map