/*!
 * ansi-grey <https://github.com/jonschlinkert/ansi-grey>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function grey(message) {
  return wrap(90, 39, message);
};
