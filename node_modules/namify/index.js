/*!
 * varname <https://github.com/jonschlinkert/varname
 *
 * Copyright (c) 2014 Jon Schlinkert, contributors
 * Licensed under the MIT License (MIT)
 */

var reserved = require('reserved');

module.exports = function (str, blacklist) {
  str = str.split(/[\W\s_-]/).filter(Boolean).join(' ')
  str = str.replace(/[\W\s_-]+(.)/g, function (match, $1) {
    return $1.toUpperCase();
  });
  // if the final word is a ECMAscript reserved word, add a leading `_`
  return (reserved.indexOf(str) !== -1) ? '_' + str : str;
};