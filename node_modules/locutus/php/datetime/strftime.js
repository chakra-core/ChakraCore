'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function strftime(fmt, timestamp) {
  //       discuss at: http://locutus.io/php/strftime/
  //      original by: Blues (http://tech.bluesmoon.info/)
  // reimplemented by: Brett Zamir (http://brett-zamir.me)
  //         input by: Alex
  //      bugfixed by: Brett Zamir (http://brett-zamir.me)
  //      improved by: Brett Zamir (http://brett-zamir.me)
  //           note 1: Uses global: locutus to store locale info
  //        example 1: strftime("%A", 1062462400); // Return value will depend on date and locale
  //        returns 1: 'Tuesday'

  var setlocale = require('../strings/setlocale');

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;

  // ensure setup of localization variables takes place
  setlocale('LC_ALL', 0);

  var _xPad = function _xPad(x, pad, r) {
    if (typeof r === 'undefined') {
      r = 10;
    }
    for (; parseInt(x, 10) < r && r > 1; r /= 10) {
      x = pad.toString() + x;
    }
    return x.toString();
  };

  var locale = $locutus.php.localeCategories.LC_TIME;
  var lcTime = $locutus.php.locales[locale].LC_TIME;

  var _formats = {
    a: function a(d) {
      return lcTime.a[d.getDay()];
    },
    A: function A(d) {
      return lcTime.A[d.getDay()];
    },
    b: function b(d) {
      return lcTime.b[d.getMonth()];
    },
    B: function B(d) {
      return lcTime.B[d.getMonth()];
    },
    C: function C(d) {
      return _xPad(parseInt(d.getFullYear() / 100, 10), 0);
    },
    d: ['getDate', '0'],
    e: ['getDate', ' '],
    g: function g(d) {
      return _xPad(parseInt(this.G(d) / 100, 10), 0);
    },
    G: function G(d) {
      var y = d.getFullYear();
      var V = parseInt(_formats.V(d), 10);
      var W = parseInt(_formats.W(d), 10);

      if (W > V) {
        y++;
      } else if (W === 0 && V >= 52) {
        y--;
      }

      return y;
    },
    H: ['getHours', '0'],
    I: function I(d) {
      var I = d.getHours() % 12;
      return _xPad(I === 0 ? 12 : I, 0);
    },
    j: function j(d) {
      var ms = d - new Date('' + d.getFullYear() + '/1/1 GMT');
      // Line differs from Yahoo implementation which would be
      // equivalent to replacing it here with:
      ms += d.getTimezoneOffset() * 60000;
      var doy = parseInt(ms / 60000 / 60 / 24, 10) + 1;
      return _xPad(doy, 0, 100);
    },
    k: ['getHours', '0'],
    // not in PHP, but implemented here (as in Yahoo)
    l: function l(d) {
      var l = d.getHours() % 12;
      return _xPad(l === 0 ? 12 : l, ' ');
    },
    m: function m(d) {
      return _xPad(d.getMonth() + 1, 0);
    },
    M: ['getMinutes', '0'],
    p: function p(d) {
      return lcTime.p[d.getHours() >= 12 ? 1 : 0];
    },
    P: function P(d) {
      return lcTime.P[d.getHours() >= 12 ? 1 : 0];
    },
    s: function s(d) {
      // Yahoo uses return parseInt(d.getTime()/1000, 10);
      return Date.parse(d) / 1000;
    },
    S: ['getSeconds', '0'],
    u: function u(d) {
      var dow = d.getDay();
      return dow === 0 ? 7 : dow;
    },
    U: function U(d) {
      var doy = parseInt(_formats.j(d), 10);
      var rdow = 6 - d.getDay();
      var woy = parseInt((doy + rdow) / 7, 10);
      return _xPad(woy, 0);
    },
    V: function V(d) {
      var woy = parseInt(_formats.W(d), 10);
      var dow11 = new Date('' + d.getFullYear() + '/1/1').getDay();
      // First week is 01 and not 00 as in the case of %U and %W,
      // so we add 1 to the final result except if day 1 of the year
      // is a Monday (then %W returns 01).
      // We also need to subtract 1 if the day 1 of the year is
      // Friday-Sunday, so the resulting equation becomes:
      var idow = woy + (dow11 > 4 || dow11 <= 1 ? 0 : 1);
      if (idow === 53 && new Date('' + d.getFullYear() + '/12/31').getDay() < 4) {
        idow = 1;
      } else if (idow === 0) {
        idow = _formats.V(new Date('' + (d.getFullYear() - 1) + '/12/31'));
      }
      return _xPad(idow, 0);
    },
    w: 'getDay',
    W: function W(d) {
      var doy = parseInt(_formats.j(d), 10);
      var rdow = 7 - _formats.u(d);
      var woy = parseInt((doy + rdow) / 7, 10);
      return _xPad(woy, 0, 10);
    },
    y: function y(d) {
      return _xPad(d.getFullYear() % 100, 0);
    },
    Y: 'getFullYear',
    z: function z(d) {
      var o = d.getTimezoneOffset();
      var H = _xPad(parseInt(Math.abs(o / 60), 10), 0);
      var M = _xPad(o % 60, 0);
      return (o > 0 ? '-' : '+') + H + M;
    },
    Z: function Z(d) {
      return d.toString().replace(/^.*\(([^)]+)\)$/, '$1');
    },
    '%': function _(d) {
      return '%';
    }
  };

  var _date = typeof timestamp === 'undefined' ? new Date() : timestamp instanceof Date ? new Date(timestamp) : new Date(timestamp * 1000);

  var _aggregates = {
    c: 'locale',
    D: '%m/%d/%y',
    F: '%y-%m-%d',
    h: '%b',
    n: '\n',
    r: 'locale',
    R: '%H:%M',
    t: '\t',
    T: '%H:%M:%S',
    x: 'locale',
    X: 'locale'
  };

  // First replace aggregates (run in a loop because an agg may be made up of other aggs)
  while (fmt.match(/%[cDFhnrRtTxX]/)) {
    fmt = fmt.replace(/%([cDFhnrRtTxX])/g, function (m0, m1) {
      var f = _aggregates[m1];
      return f === 'locale' ? lcTime[m1] : f;
    });
  }

  // Now replace formats - we need a closure so that the date object gets passed through
  var str = fmt.replace(/%([aAbBCdegGHIjklmMpPsSuUVwWyYzZ%])/g, function (m0, m1) {
    var f = _formats[m1];
    if (typeof f === 'string') {
      return _date[f]();
    } else if (typeof f === 'function') {
      return f(_date);
    } else if ((typeof f === 'undefined' ? 'undefined' : _typeof(f)) === 'object' && typeof f[0] === 'string') {
      return _xPad(_date[f[0]](), f[1]);
    } else {
      // Shouldn't reach here
      return m1;
    }
  });

  return str;
};
//# sourceMappingURL=strftime.js.map