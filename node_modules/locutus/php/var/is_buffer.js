'use strict';

module.exports = function is_buffer(vr) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/is_buffer/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: is_buffer('This could be binary or a regular string...')
  //   returns 1: true

  return typeof vr === 'string';
};
//# sourceMappingURL=is_buffer.js.map