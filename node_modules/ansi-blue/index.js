/*!
 * ansi-blue <https://github.com/jonschlinkert/ansi-blue>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function blue(message) {
  return wrap(34, 39, message);
};
