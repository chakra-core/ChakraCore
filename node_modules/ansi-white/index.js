/*!
 * ansi-white <https://github.com/jonschlinkert/ansi-white>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function white(message) {
  return wrap(37, 39, message);
};
