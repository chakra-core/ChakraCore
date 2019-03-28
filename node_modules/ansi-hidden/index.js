/*!
 * ansi-hidden <https://github.com/jonschlinkert/ansi-hidden>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function hidden(message) {
  return wrap(8, 28, message);
};
