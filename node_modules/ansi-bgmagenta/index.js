/*!
 * ansi-bgmagenta <https://github.com/jonschlinkert/ansi-bgmagenta>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function bgmagenta(message) {
  return wrap(45, 49, message);
};
