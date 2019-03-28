'use strict';

var util = require('util');
var debug = require('debug');
var field = debug('base:config:process:field');

module.exports = function(prop) {
  return debug('base:config:process:' + prop);
};

module.exports.schema = debug('base:config:process');
module.exports.field = function(key, val) {
  field('processing ' + key + ', ' + util.inspect(val, null, 10));
};
