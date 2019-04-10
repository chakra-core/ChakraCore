'use strict';

var util = require('util');
var debug = require('debug');
var field = debug('base:config:schema');

module.exports.schema = debug('base:config:schema');
module.exports.field = function(key, val) {
  field('normalizing ' + key + ', ' + util.inspect(val, null, 10));
};
