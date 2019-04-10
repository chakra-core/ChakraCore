'use strict';

module.exports = function strtok(str, tokens) {
  //  discuss at: http://locutus.io/php/strtok/
  // original by: Brett Zamir (http://brett-zamir.me)
  //      note 1: Use tab and newline as tokenizing characters as well
  //   example 1: var $string = "\t\t\t\nThis is\tan example\nstring\n"
  //   example 1: var $tok = strtok($string, " \n\t")
  //   example 1: var $b = ''
  //   example 1: while ($tok !== false) {$b += "Word="+$tok+"\n"; $tok = strtok(" \n\t");}
  //   example 1: var $result = $b
  //   returns 1: "Word=This\nWord=is\nWord=an\nWord=example\nWord=string\n"

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};

  if (tokens === undefined) {
    tokens = str;
    str = $locutus.php.strtokleftOver;
  }
  if (str.length === 0) {
    return false;
  }
  if (tokens.indexOf(str.charAt(0)) !== -1) {
    return strtok(str.substr(1), tokens);
  }
  for (var i = 0; i < str.length; i++) {
    if (tokens.indexOf(str.charAt(i)) !== -1) {
      break;
    }
  }
  $locutus.php.strtokleftOver = str.substr(i + 1);

  return str.substring(0, i);
};
//# sourceMappingURL=strtok.js.map