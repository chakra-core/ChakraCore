/*!
 * ansi-italic <https://github.com/jonschlinkert/ansi-italic>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function italic(message) {
  return wrap(3, 23, message);
};
