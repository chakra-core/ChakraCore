'use strict';

module.exports = function quotemeta(str) {
  //  discuss at: http://locutus.io/php/quotemeta/
  // original by: Paulo Freitas
  //   example 1: quotemeta(". + * ? ^ ( $ )")
  //   returns 1: '\\. \\+ \\* \\? \\^ \\( \\$ \\)'

  return (str + '').replace(/([.\\+*?[^\]$()])/g, '\\$1');
};
//# sourceMappingURL=quotemeta.js.map