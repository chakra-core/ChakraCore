/*!
 * ansi-magenta <https://github.com/jonschlinkert/ansi-magenta>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function magenta(message) {
  return wrap(35, 39, message);
};
