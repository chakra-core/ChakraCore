/*!
 * ansi-underline <https://github.com/jonschlinkert/ansi-underline>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function underline(message) {
  return wrap(4, 24, message);
};
