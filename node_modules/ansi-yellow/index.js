/*!
 * ansi-yellow <https://github.com/jonschlinkert/ansi-yellow>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function yellow(message) {
  return wrap(33, 39, message);
};
