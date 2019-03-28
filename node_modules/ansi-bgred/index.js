/*!
 * ansi-bgred <https://github.com/jonschlinkert/ansi-bgred>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function bgred(message) {
  return wrap(41, 49, message);
};
