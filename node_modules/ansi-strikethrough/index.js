/*!
 * ansi-strikethrough <https://github.com/jonschlinkert/ansi-strikethrough>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function strikethrough(message) {
  return wrap(9, 29, message);
};
