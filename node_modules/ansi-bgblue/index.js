/*!
 * ansi-bgblue <https://github.com/jonschlinkert/ansi-bgblue>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function bgblue(message) {
  return wrap(44, 49, message);
};
