'use strict';

var util = require('util');
var debug = require('debug');
var field = debug('base:cli:process:field');

module.exports.field = function(key, val) {
  field('processing ' + key + ': ' + util.inspect(val, null, 10));
};

