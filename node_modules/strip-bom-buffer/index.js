/*!
 * strip-bom-buffer <https://github.com/jonschlinkert/strip-bom-buffer>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isBuffer = require('is-buffer');
var isUtf8 = require('is-utf8');

module.exports = function (buffer) {
  if (!isBuffer(buffer) || !isUtf8(buffer)) {
    return buffer;
  }
  if (String(buffer.slice(0, 3)) === '\ufeff') {
    return buffer.slice(3);
  }
  return buffer;
};
