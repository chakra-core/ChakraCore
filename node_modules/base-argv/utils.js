'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('arr-diff', 'diff');
require('arr-union', 'union');
require('define-property', 'define');
require('expand-args');
require('extend-shallow', 'extend');
require = fn;

/**
 * Cast the given `value` to an array.
 *
 * ```js
 * utils.arrayify('foo');
 * //=> ['foo']
 *
 * utils.arrayify(['foo']);
 * //=> ['foo']
 * ```
 * @param {String|Array} `value`
 * @return {Array}
 * @api public
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
