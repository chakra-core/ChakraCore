/*!
 * ansi-bgblack <https://github.com/jonschlinkert/ansi-bgblack>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function bgblack(message) {
  return wrap(40, 49, message);
};
