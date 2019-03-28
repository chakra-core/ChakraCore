/*!
 * ansi-bgwhite <https://github.com/jonschlinkert/ansi-bgwhite>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function bgwhite(message) {
  return wrap(47, 49, message);
};
