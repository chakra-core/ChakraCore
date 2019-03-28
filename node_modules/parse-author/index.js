/*!
 * parse-author <https://github.com/jonschlinkert/parse-author>
 *
 * Copyright (c) 2014-2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

module.exports = function(str) {
  if (typeof str !== 'string') {
    throw new TypeError('expected author to be a string');
  }
  var name = str.match(/^([^\(<]+)/);
  var url = str.match(/\(([^\)]+)\)/);
  var email = str.match(/<([^>]+)>/);
  var obj = {};
  if (name && name[1].trim()) obj.name = name[1].trim();
  if (email && email[1].trim()) obj.email = email[1].trim();
  if (url && url[1].trim()) obj.url = url[1].trim();
  return obj;
};
