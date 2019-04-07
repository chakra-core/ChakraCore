/*!
 * sort-asc <https://github.com/helpers/sort-asc>
 *
 * Copyright (c) 2014-2015 Jon Schlinkert.
 * Licensed under the MIT License
 */

'use strict';

module.exports = function (a, b) {
  return a === b ? 0 : a.localeCompare(b);
};