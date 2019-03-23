/*!
 * is-scaffold <https://github.com/jonschlinkert/is-scaffold>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isPlainObject = require('is-plain-object');

module.exports = function isScaffold(obj) {
  return obj && typeof obj === 'object'
    && !isPlainObject(obj)
    && obj.isScaffold === true;
};
