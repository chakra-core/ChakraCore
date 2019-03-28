/*!
 * ansi-black <https://github.com/jonschlinkert/ansi-black>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function black(message) {
  return wrap(30, 39, message);
};
