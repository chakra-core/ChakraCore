'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('extend-shallow', 'extend');
require('file-is-binary', 'isBinary');
require('gray-matter', 'matter');
require('mixin-deep', 'merge');
require('isobject', 'isObject');
require('trim-leading-lines', 'trim');
require = fn;

utils.isValidFile = function(file) {
  file = file || {};
  return !utils.isNull(file) && !utils.isBinary(file);
};

utils.hasYFM = function(file) {
  return file.data && file.data.hasOwnProperty('yfm');
};

utils.isNull = function isNull(file) {
  if (file && typeof file.isNull !== 'function') {
    file.isNull = function() {
      return file.contents === null;
    };
  }
  return file.isNull();
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
