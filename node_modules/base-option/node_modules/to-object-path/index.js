/*!
 * to-object-path <https://github.com/jonschlinkert/to-object-path>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isArguments = require('is-arguments');
var flatten = require('arr-flatten');

module.exports = function toPath(args) {
  if (isArguments(args)) {
    args = [].slice.call(args);
  } else {
    args = [].slice.call(arguments);
  }
  return flatten(args).join('.');
};
