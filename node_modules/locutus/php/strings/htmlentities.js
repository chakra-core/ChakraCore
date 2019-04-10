'use strict';

module.exports = function htmlentities(string, quoteStyle, charset, doubleEncode) {
  //  discuss at: http://locutus.io/php/htmlentities/
  // original by: Kevin van Zonneveld (http://kvz.io)
  //  revised by: Kevin van Zonneveld (http://kvz.io)
  //  revised by: Kevin van Zonneveld (http://kvz.io)
  // improved by: nobbler
  // improved by: Jack
  // improved by: Rafał Kukawski (http://blog.kukawski.pl)
  // improved by: Dj (http://locutus.io/php/htmlentities:425#comment_134018)
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  //    input by: Ratheous
  //      note 1: function is compatible with PHP 5.2 and older
  //   example 1: htmlentities('Kevin & van Zonneveld')
  //   returns 1: 'Kevin &amp; van Zonneveld'
  //   example 2: htmlentities("foo'bar","ENT_QUOTES")
  //   returns 2: 'foo&#039;bar'

  var getHtmlTranslationTable = require('../strings/get_html_translation_table');
  var hashMap = getHtmlTranslationTable('HTML_ENTITIES', quoteStyle);

  string = string === null ? '' : string + '';

  if (!hashMap) {
    return false;
  }

  if (quoteStyle && quoteStyle === 'ENT_QUOTES') {
    hashMap["'"] = '&#039;';
  }

  doubleEncode = doubleEncode === null || !!doubleEncode;

  var regex = new RegExp('&(?:#\\d+|#x[\\da-f]+|[a-zA-Z][\\da-z]*);|[' + Object.keys(hashMap).join('')
  // replace regexp special chars
  .replace(/([()[\]{}\-.*+?^$|/\\])/g, '\\$1') + ']', 'g');

  return string.replace(regex, function (ent) {
    if (ent.length > 1) {
      return doubleEncode ? hashMap['&'] + ent.substr(1) : ent;
    }

    return hashMap[ent];
  });
};
//# sourceMappingURL=htmlentities.js.map