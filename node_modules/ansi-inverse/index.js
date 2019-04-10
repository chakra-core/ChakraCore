/*!
 * ansi-inverse <https://github.com/jonschlinkert/ansi-inverse>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function inverse(message) {
  return wrap(7, 27, message);
};
