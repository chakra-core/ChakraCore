/*!
 * ansi-bggreen <https://github.com/jonschlinkert/ansi-bggreen>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function bggreen(message) {
  return wrap(42, 49, message);
};
