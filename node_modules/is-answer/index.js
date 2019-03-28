/*!
 * is-answer <https://github.com/jonschlinkert/is-answer>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isPrimitive = require('is-primitive');
var omitEmpty = require('omit-empty');
var has = require('has-values');

module.exports = function(answer) {
  return isPrimitive(answer) ? has(answer) : has(omitEmpty(answer));
};

