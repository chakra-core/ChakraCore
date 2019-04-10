'use strict';

/**
 * Lazily required module dependencies
 */

var utils = require('lazy-cache')(require);
var fn = require;

require = utils;
require('arr-flatten', 'flatten');
require('collection-visit', 'visit');
require('get-value', 'get');
require('has-value', 'has');
require('kind-of', 'typeOf');
require('set-value', 'set');
require('to-object-path', 'toPath');
require = fn;

utils.mergeArray = function(arr, fn, context) {
  var len = arr.length;
  var idx = -1;
  while (++idx < len) {
    var arg = arr[idx];
    var keys = Object.keys(arg);

    for (var i = 0; i < keys.length; i++) {
      var key = keys[i];
      fn.call(context, arg[key], key, arg);
    }
  }
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
