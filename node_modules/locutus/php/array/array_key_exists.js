"use strict";

module.exports = function array_key_exists(key, search) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/array_key_exists/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Felix Geisendoerfer (http://www.debuggable.com/felix)
  //   example 1: array_key_exists('kevin', {'kevin': 'van Zonneveld'})
  //   returns 1: true

  if (!search || search.constructor !== Array && search.constructor !== Object) {
    return false;
  }

  return key in search;
};
//# sourceMappingURL=array_key_exists.js.map