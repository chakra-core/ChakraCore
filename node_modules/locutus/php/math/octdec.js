'use strict';

module.exports = function octdec(octString) {
  //  discuss at: http://locutus.io/php/octdec/
  // original by: Philippe Baumann
  //   example 1: octdec('77')
  //   returns 1: 63

  octString = (octString + '').replace(/[^0-7]/gi, '');
  return parseInt(octString, 8);
};
//# sourceMappingURL=octdec.js.map