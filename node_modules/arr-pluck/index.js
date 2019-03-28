/*!
 * arr-pluck <https://github.com/jonschlinkert/arr-pluck>
 *
 * Copyright (c) 2014-2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var map = require('arr-map');

module.exports = function pluck(arr, prop) {
  return map(arr, prop);
};
