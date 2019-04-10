/*!
 * base-cli <https://github.com/jonschlinkert/base-cli>
 *
 * Copyright (c) 2015-2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var config = require('base-config');
var argv = require('base-argv');

module.exports = function(options) {
  return function(app, base) {
    this.use(argv(options));
    this.use(config.create('cli'));
  };
};
