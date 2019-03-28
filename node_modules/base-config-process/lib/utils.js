'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('base-config', 'config');
require('base-config-schema', 'schema');
require('base-cwd', 'cwd');
require('base-option', 'option');
require('is-valid-app', 'isValid');
require('micromatch', 'mm');
require('mixin-deep', 'merge');
require = fn;

utils.toRegex = function(val) {
  if (val instanceof RegExp) {
    return val;
  }
  if (typeof val === 'string') {
    var opts = {};
    if (val.indexOf('**') === -1 && !/[\\\/]/.test(val)) {
      val = '**/' + val;
    }
    return utils.mm.makeRe(val, opts);
  }
  return /./;
};

/**
 * Expose `utils`
 */

module.exports = utils;
