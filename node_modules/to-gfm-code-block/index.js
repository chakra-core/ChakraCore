/*!
 * to-gfm-code-block <https://github.com/jonschlinkert/to-gfm-code-block>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

module.exports = function pre(str, lang) {
  if (typeof str !== 'string') {
    throw new TypeError('markdown-pre expects a string.');
  }

  var code = '';
  code += '```' + (typeof lang === 'string' ? lang : '');
  code += '\n';
  code += str;
  code += '\n';
  code += '```';
  return code;
};
