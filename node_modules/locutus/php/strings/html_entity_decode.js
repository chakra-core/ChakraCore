'use strict';

module.exports = function html_entity_decode(string, quoteStyle) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/html_entity_decode/
  // original by: john (http://www.jd-tech.net)
  //    input by: ger
  //    input by: Ratheous
  //    input by: Nick Kolosov (http://sammy.ru)
  // improved by: Kevin van Zonneveld (http://kvz.io)
  // improved by: marc andreu
  //  revised by: Kevin van Zonneveld (http://kvz.io)
  //  revised by: Kevin van Zonneveld (http://kvz.io)
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: Fox
  //   example 1: html_entity_decode('Kevin &amp; van Zonneveld')
  //   returns 1: 'Kevin & van Zonneveld'
  //   example 2: html_entity_decode('&amp;lt;')
  //   returns 2: '&lt;'

  var getHtmlTranslationTable = require('../strings/get_html_translation_table');
  var tmpStr = '';
  var entity = '';
  var symbol = '';
  tmpStr = string.toString();

  var hashMap = getHtmlTranslationTable('HTML_ENTITIES', quoteStyle);
  if (hashMap === false) {
    return false;
  }

  // @todo: &amp; problem
  // http://locutus.io/php/get_html_translation_table:416#comment_97660
  delete hashMap['&'];
  hashMap['&'] = '&amp;';

  for (symbol in hashMap) {
    entity = hashMap[symbol];
    tmpStr = tmpStr.split(entity).join(symbol);
  }
  tmpStr = tmpStr.split('&#039;').join("'");

  return tmpStr;
};
//# sourceMappingURL=html_entity_decode.js.map