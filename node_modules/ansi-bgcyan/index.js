/*!
 * ansi-bgcyan <https://github.com/jonschlinkert/ansi-bgcyan>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function bgcyan(message) {
  return wrap(46, 49, message);
};
