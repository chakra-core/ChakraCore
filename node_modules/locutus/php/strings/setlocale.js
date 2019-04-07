'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function setlocale(category, locale) {
  //  discuss at: http://locutus.io/php/setlocale/
  // original by: Brett Zamir (http://brett-zamir.me)
  // original by: Blues (http://hacks.bluesmoon.info/strftime/strftime.js)
  // original by: YUI Library (http://developer.yahoo.com/yui/docs/YAHOO.util.DateLocale.html)
  //      note 1: Is extensible, but currently only implements locales en,
  //      note 1: en_US, en_GB, en_AU, fr, and fr_CA for LC_TIME only; C for LC_CTYPE;
  //      note 1: C and en for LC_MONETARY/LC_NUMERIC; en for LC_COLLATE
  //      note 1: Uses global: locutus to store locale info
  //      note 1: Consider using http://demo.icu-project.org/icu-bin/locexp as basis for localization (as in i18n_loc_set_default())
  //      note 2: This function tries to establish the locale via the `window` global.
  //      note 2: This feature will not work in Node and hence is Browser-only
  //   example 1: setlocale('LC_ALL', 'en_US')
  //   returns 1: 'en_US'

  var getenv = require('../info/getenv');

  var categ = '';
  var cats = [];
  var i = 0;

  var _copy = function _copy(orig) {
    if (orig instanceof RegExp) {
      return new RegExp(orig);
    } else if (orig instanceof Date) {
      return new Date(orig);
    }
    var newObj = {};
    for (var i in orig) {
      if (_typeof(orig[i]) === 'object') {
        newObj[i] = _copy(orig[i]);
      } else {
        newObj[i] = orig[i];
      }
    }
    return newObj;
  };

  // Function usable by a ngettext implementation (apparently not an accessible part of setlocale(),
  // but locale-specific) See http://www.gnu.org/software/gettext/manual/gettext.html#Plural-forms
  // though amended with others from https://developer.mozilla.org/En/Localization_and_Plurals (new
  // categories noted with "MDC" below, though not sure of whether there is a convention for the
  // relative order of these newer groups as far as ngettext) The function name indicates the number
  // of plural forms (nplural) Need to look into http://cldr.unicode.org/ (maybe future JavaScript);
  // Dojo has some functions (under new BSD), including JSON conversions of LDML XML from CLDR:
  // http://bugs.dojotoolkit.org/browser/dojo/trunk/cldr and docs at
  // http://api.dojotoolkit.org/jsdoc/HEAD/dojo.cldr

  // var _nplurals1 = function (n) {
  //   // e.g., Japanese
  //   return 0
  // }
  var _nplurals2a = function _nplurals2a(n) {
    // e.g., English
    return n !== 1 ? 1 : 0;
  };
  var _nplurals2b = function _nplurals2b(n) {
    // e.g., French
    return n > 1 ? 1 : 0;
  };

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};

  // Reconcile Windows vs. *nix locale names?
  // Allow different priority orders of languages, esp. if implement gettext as in
  // LANGUAGE env. var.? (e.g., show German if French is not available)
  if (!$locutus.php.locales || !$locutus.php.locales.fr_CA || !$locutus.php.locales.fr_CA.LC_TIME || !$locutus.php.locales.fr_CA.LC_TIME.x) {
    // Can add to the locales
    $locutus.php.locales = {};

    $locutus.php.locales.en = {
      'LC_COLLATE': function LC_COLLATE(str1, str2) {
        // @todo: This one taken from strcmp, but need for other locales; we don't use localeCompare
        // since its locale is not settable
        return str1 === str2 ? 0 : str1 > str2 ? 1 : -1;
      },
      'LC_CTYPE': {
        // Need to change any of these for English as opposed to C?
        an: /^[A-Za-z\d]+$/g,
        al: /^[A-Za-z]+$/g,
        ct: /^[\u0000-\u001F\u007F]+$/g,
        dg: /^[\d]+$/g,
        gr: /^[\u0021-\u007E]+$/g,
        lw: /^[a-z]+$/g,
        pr: /^[\u0020-\u007E]+$/g,
        pu: /^[\u0021-\u002F\u003A-\u0040\u005B-\u0060\u007B-\u007E]+$/g,
        sp: /^[\f\n\r\t\v ]+$/g,
        up: /^[A-Z]+$/g,
        xd: /^[A-Fa-f\d]+$/g,
        CODESET: 'UTF-8',
        // Used by sql_regcase
        lower: 'abcdefghijklmnopqrstuvwxyz',
        upper: 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
      },
      'LC_TIME': {
        // Comments include nl_langinfo() constant equivalents and any
        // changes from Blues' implementation
        a: ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'],
        // ABDAY_
        A: ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'],
        // DAY_
        b: ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'],
        // ABMON_
        B: ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'],
        // MON_
        c: '%a %d %b %Y %r %Z',
        // D_T_FMT // changed %T to %r per results
        p: ['AM', 'PM'],
        // AM_STR/PM_STR
        P: ['am', 'pm'],
        // Not available in nl_langinfo()
        r: '%I:%M:%S %p',
        // T_FMT_AMPM (Fixed for all locales)
        x: '%m/%d/%Y',
        // D_FMT // switched order of %m and %d; changed %y to %Y (C uses %y)
        X: '%r',
        // T_FMT // changed from %T to %r  (%T is default for C, not English US)
        // Following are from nl_langinfo() or http://www.cptec.inpe.br/sx4/sx4man2/g1ab02e/strftime.4.html
        alt_digits: '',
        // e.g., ordinal
        ERA: '',
        ERA_YEAR: '',
        ERA_D_T_FMT: '',
        ERA_D_FMT: '',
        ERA_T_FMT: ''
      },
      // Assuming distinction between numeric and monetary is thus:
      // See below for C locale
      'LC_MONETARY': {
        // based on Windows "english" (English_United States.1252) locale
        int_curr_symbol: 'USD',
        currency_symbol: '$',
        mon_decimal_point: '.',
        mon_thousands_sep: ',',
        mon_grouping: [3],
        // use mon_thousands_sep; "" for no grouping; additional array members
        // indicate successive group lengths after first group
        // (e.g., if to be 1,23,456, could be [3, 2])
        positive_sign: '',
        negative_sign: '-',
        int_frac_digits: 2,
        // Fractional digits only for money defaults?
        frac_digits: 2,
        p_cs_precedes: 1,
        // positive currency symbol follows value = 0; precedes value = 1
        p_sep_by_space: 0,
        // 0: no space between curr. symbol and value; 1: space sep. them unless symb.
        // and sign are adjacent then space sep. them from value; 2: space sep. sign
        // and value unless symb. and sign are adjacent then space separates
        n_cs_precedes: 1,
        // see p_cs_precedes
        n_sep_by_space: 0,
        // see p_sep_by_space
        p_sign_posn: 3,
        // 0: parentheses surround quantity and curr. symbol; 1: sign precedes them;
        // 2: sign follows them; 3: sign immed. precedes curr. symbol; 4: sign immed.
        // succeeds curr. symbol
        n_sign_posn: 0 // see p_sign_posn
      },
      'LC_NUMERIC': {
        // based on Windows "english" (English_United States.1252) locale
        decimal_point: '.',
        thousands_sep: ',',
        grouping: [3] // see mon_grouping, but for non-monetary values (use thousands_sep)
      },
      'LC_MESSAGES': {
        YESEXPR: '^[yY].*',
        NOEXPR: '^[nN].*',
        YESSTR: '',
        NOSTR: ''
      },
      nplurals: _nplurals2a
    };
    $locutus.php.locales.en_US = _copy($locutus.php.locales.en);
    $locutus.php.locales.en_US.LC_TIME.c = '%a %d %b %Y %r %Z';
    $locutus.php.locales.en_US.LC_TIME.x = '%D';
    $locutus.php.locales.en_US.LC_TIME.X = '%r';
    // The following are based on *nix settings
    $locutus.php.locales.en_US.LC_MONETARY.int_curr_symbol = 'USD ';
    $locutus.php.locales.en_US.LC_MONETARY.p_sign_posn = 1;
    $locutus.php.locales.en_US.LC_MONETARY.n_sign_posn = 1;
    $locutus.php.locales.en_US.LC_MONETARY.mon_grouping = [3, 3];
    $locutus.php.locales.en_US.LC_NUMERIC.thousands_sep = '';
    $locutus.php.locales.en_US.LC_NUMERIC.grouping = [];

    $locutus.php.locales.en_GB = _copy($locutus.php.locales.en);
    $locutus.php.locales.en_GB.LC_TIME.r = '%l:%M:%S %P %Z';

    $locutus.php.locales.en_AU = _copy($locutus.php.locales.en_GB);
    // Assume C locale is like English (?) (We need C locale for LC_CTYPE)
    $locutus.php.locales.C = _copy($locutus.php.locales.en);
    $locutus.php.locales.C.LC_CTYPE.CODESET = 'ANSI_X3.4-1968';
    $locutus.php.locales.C.LC_MONETARY = {
      int_curr_symbol: '',
      currency_symbol: '',
      mon_decimal_point: '',
      mon_thousands_sep: '',
      mon_grouping: [],
      p_cs_precedes: 127,
      p_sep_by_space: 127,
      n_cs_precedes: 127,
      n_sep_by_space: 127,
      p_sign_posn: 127,
      n_sign_posn: 127,
      positive_sign: '',
      negative_sign: '',
      int_frac_digits: 127,
      frac_digits: 127
    };
    $locutus.php.locales.C.LC_NUMERIC = {
      decimal_point: '.',
      thousands_sep: '',
      grouping: []
    };
    // D_T_FMT
    $locutus.php.locales.C.LC_TIME.c = '%a %b %e %H:%M:%S %Y';
    // D_FMT
    $locutus.php.locales.C.LC_TIME.x = '%m/%d/%y';
    // T_FMT
    $locutus.php.locales.C.LC_TIME.X = '%H:%M:%S';
    $locutus.php.locales.C.LC_MESSAGES.YESEXPR = '^[yY]';
    $locutus.php.locales.C.LC_MESSAGES.NOEXPR = '^[nN]';

    $locutus.php.locales.fr = _copy($locutus.php.locales.en);
    $locutus.php.locales.fr.nplurals = _nplurals2b;
    $locutus.php.locales.fr.LC_TIME.a = ['dim', 'lun', 'mar', 'mer', 'jeu', 'ven', 'sam'];
    $locutus.php.locales.fr.LC_TIME.A = ['dimanche', 'lundi', 'mardi', 'mercredi', 'jeudi', 'vendredi', 'samedi'];
    $locutus.php.locales.fr.LC_TIME.b = ['jan', 'f\xE9v', 'mar', 'avr', 'mai', 'jun', 'jui', 'ao\xFB', 'sep', 'oct', 'nov', 'd\xE9c'];
    $locutus.php.locales.fr.LC_TIME.B = ['janvier', 'f\xE9vrier', 'mars', 'avril', 'mai', 'juin', 'juillet', 'ao\xFBt', 'septembre', 'octobre', 'novembre', 'd\xE9cembre'];
    $locutus.php.locales.fr.LC_TIME.c = '%a %d %b %Y %T %Z';
    $locutus.php.locales.fr.LC_TIME.p = ['', ''];
    $locutus.php.locales.fr.LC_TIME.P = ['', ''];
    $locutus.php.locales.fr.LC_TIME.x = '%d.%m.%Y';
    $locutus.php.locales.fr.LC_TIME.X = '%T';

    $locutus.php.locales.fr_CA = _copy($locutus.php.locales.fr);
    $locutus.php.locales.fr_CA.LC_TIME.x = '%Y-%m-%d';
  }
  if (!$locutus.php.locale) {
    $locutus.php.locale = 'en_US';
    // Try to establish the locale via the `window` global
    if (typeof window !== 'undefined' && window.document) {
      var d = window.document;
      var NS_XHTML = 'http://www.w3.org/1999/xhtml';
      var NS_XML = 'http://www.w3.org/XML/1998/namespace';
      if (d.getElementsByTagNameNS && d.getElementsByTagNameNS(NS_XHTML, 'html')[0]) {
        if (d.getElementsByTagNameNS(NS_XHTML, 'html')[0].getAttributeNS && d.getElementsByTagNameNS(NS_XHTML, 'html')[0].getAttributeNS(NS_XML, 'lang')) {
          $locutus.php.locale = d.getElementsByTagName(NS_XHTML, 'html')[0].getAttributeNS(NS_XML, 'lang');
        } else if (d.getElementsByTagNameNS(NS_XHTML, 'html')[0].lang) {
          // XHTML 1.0 only
          $locutus.php.locale = d.getElementsByTagNameNS(NS_XHTML, 'html')[0].lang;
        }
      } else if (d.getElementsByTagName('html')[0] && d.getElementsByTagName('html')[0].lang) {
        $locutus.php.locale = d.getElementsByTagName('html')[0].lang;
      }
    }
  }
  // PHP-style
  $locutus.php.locale = $locutus.php.locale.replace('-', '_');
  // @todo: locale if declared locale hasn't been defined
  if (!($locutus.php.locale in $locutus.php.locales)) {
    if ($locutus.php.locale.replace(/_[a-zA-Z]+$/, '') in $locutus.php.locales) {
      $locutus.php.locale = $locutus.php.locale.replace(/_[a-zA-Z]+$/, '');
    }
  }

  if (!$locutus.php.localeCategories) {
    $locutus.php.localeCategories = {
      'LC_COLLATE': $locutus.php.locale,
      // for string comparison, see strcoll()
      'LC_CTYPE': $locutus.php.locale,
      // for character classification and conversion, for example strtoupper()
      'LC_MONETARY': $locutus.php.locale,
      // for localeconv()
      'LC_NUMERIC': $locutus.php.locale,
      // for decimal separator (See also localeconv())
      'LC_TIME': $locutus.php.locale,
      // for date and time formatting with strftime()
      // for system responses (available if PHP was compiled with libintl):
      'LC_MESSAGES': $locutus.php.locale
    };
  }

  if (locale === null || locale === '') {
    locale = getenv(category) || getenv('LANG');
  } else if (Object.prototype.toString.call(locale) === '[object Array]') {
    for (i = 0; i < locale.length; i++) {
      if (!(locale[i] in $locutus.php.locales)) {
        if (i === locale.length - 1) {
          // none found
          return false;
        }
        continue;
      }
      locale = locale[i];
      break;
    }
  }

  // Just get the locale
  if (locale === '0' || locale === 0) {
    if (category === 'LC_ALL') {
      for (categ in $locutus.php.localeCategories) {
        // Add ".UTF-8" or allow ".@latint", etc. to the end?
        cats.push(categ + '=' + $locutus.php.localeCategories[categ]);
      }
      return cats.join(';');
    }
    return $locutus.php.localeCategories[category];
  }

  if (!(locale in $locutus.php.locales)) {
    // Locale not found
    return false;
  }

  // Set and get locale
  if (category === 'LC_ALL') {
    for (categ in $locutus.php.localeCategories) {
      $locutus.php.localeCategories[categ] = locale;
    }
  } else {
    $locutus.php.localeCategories[category] = locale;
  }

  return locale;
};
//# sourceMappingURL=setlocale.js.map