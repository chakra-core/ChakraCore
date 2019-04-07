'use strict';

module.exports = function capwords(str) {
  //  discuss at: http://locutus.io/python/capwords/
  // original by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
  // improved by: Waldo Malqui Silva (http://waldo.malqui.info)
  // improved by: Robin
  // improved by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  //    input by: James (http://www.james-bell.co.uk/)
  //   example 1: capwords('kevin van  zonneveld')
  //   returns 1: 'Kevin Van  Zonneveld'
  //   example 2: capwords('HELLO WORLD')
  //   returns 2: 'HELLO WORLD'

  var pattern = /^([a-z\u00E0-\u00FC])|\s+([a-z\u00E0-\u00FC])/g;
  return (str + '').replace(pattern, function ($1) {
    return $1.toUpperCase();
  });
};
//# sourceMappingURL=capwords.js.map