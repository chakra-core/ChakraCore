/*!
 * ansi-bold <https://github.com/jonschlinkert/ansi-bold>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function bold(message) {
  return wrap(1, 22, message);
};
