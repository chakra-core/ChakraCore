'use strict';

module.exports = function setrawcookie(name, value, expires, path, domain, secure) {
  //  discuss at: http://locutus.io/php/setrawcookie/
  // original by: Brett Zamir (http://brett-zamir.me)
  // original by: setcookie
  // improved by: Kevin van Zonneveld (http://kvz.io)
  //    input by: Michael
  //      note 1: This function requires access to the `window` global and is Browser-only
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  //   example 1: setrawcookie('author_name', 'Kevin van Zonneveld')
  //   returns 1: true

  if (typeof window === 'undefined') {
    return true;
  }

  if (typeof expires === 'string' && /^\d+$/.test(expires)) {
    expires = parseInt(expires, 10);
  }

  if (expires instanceof Date) {
    expires = expires.toUTCString();
  } else if (typeof expires === 'number') {
    expires = new Date(expires * 1e3).toUTCString();
  }

  var r = [name + '=' + value];
  var i = '';
  var s = {
    expires: expires,
    path: path,
    domain: domain
  };
  for (i in s) {
    if (s.hasOwnProperty(i)) {
      // Exclude items on Object.prototype
      s[i] && r.push(i + '=' + s[i]);
    }
  }

  if (secure) {
    r.push('secure');
  }

  window.document.cookie = r.join(';');

  return true;
};
//# sourceMappingURL=setrawcookie.js.map