'use strict';

var utils = require('lazy-cache')(require);
var fn = require;

/**
 * Lazily required module dependencies
 */

require = utils;
require('common-config', 'common');
require('data-store', 'Store');
require('is-answer');
require('project-name', 'project');
require = fn;

utils.sync = function(obj, prop, val) {
  var cached;
  Object.defineProperty(obj, prop, {
    configurable: true,
    enumerable: true,
    set: function(v) {
      cached = v;
    },
    get: function() {
      if (cached) return cached;
      if (typeof val === 'function') {
        val = val.call(obj);
      }
      cached = val;
      return val;
    }
  });
};

utils.slugify = function(str) {
  return str.split(/\W/).join('-');
};

/**
 * Expose `utils`
 */

module.exports = utils;
