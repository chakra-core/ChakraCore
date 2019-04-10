/*!
 * ansi-dim <https://github.com/jonschlinkert/ansi-dim>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var wrap = require('ansi-wrap');

module.exports = function dim(message) {
  return wrap(2, 22, message);
};
