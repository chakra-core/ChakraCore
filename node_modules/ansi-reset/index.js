/*!
 * ansi-reset <https://github.com/jonschlinkert/ansi-reset>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function reset(message) {
  return wrap(0, 0, message);
};
