'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('common-middleware', 'middleware');
require('common-questions', 'questions');
require('empty-dir');
require('extend-shallow', 'extend');
require('fs-exists-sync', 'exists');
require('is-valid-app', 'isValid');
require = fn;

utils.isEmpty = function(filepath) {
  if (!utils.exists(filepath)) return true;
  return utils.emptyDir.sync(filepath, function(fp) {
    return !/(\.DS_Store|Thumbs\.db)/.test(fp);
  });
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
