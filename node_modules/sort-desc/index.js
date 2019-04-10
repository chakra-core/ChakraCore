/*!
 * sort-desc <https://github.com/helpers/sort-desc>
 *
 * Copyright (c) 2014-2015 Jon Schlinkert.
 * Licensed under the MIT License
 */

'use strict';

module.exports = function (a, b) {
  return a === b ? 0 : b.localeCompare(a);
};