/*!
 * stringify-author <https://github.com/jonschlinkert/stringify-author>
 *
 * Copyright (c) 2014-2015 Jon Schlinkert.
 * Licensed under the MIT license.
 */

'use strict';

module.exports = function (author) {
  if (typeof author !== 'object') {
    throw new Error('expected an author to be an object');
  }

  var tmpl = {name: ['', ''], email: ['<', '>'], url: ['(', ')']};
  var str = '';

  if (author.url) author.url = stripSlash(author.url);

  for (var key in tmpl) {
    if (author[key]) {
      str += tmpl[key][0] + author[key] + tmpl[key][1] + ' ';
    }
  }
  return str.trim();
};

function stripSlash(str) {
  return str.replace(/\/$/, '');
}
