'use strict';

var util = require('util');
var debug = require('debug');
var field = debug('base:cli:schema:field');
var schema = debug('base:cli:schema');

module.exports.schema = schema;
module.exports.field = function(key, val) {
  if (typeof val !== 'undefined') {
    schema('normalizing > ' + key + ': ' + util.inspect(val, null, 10));
  }
};
module.exports.results = function(key, val) {
  if (typeof val !== 'undefined') {
    field('results > ' + key + ': ' + util.inspect(val, null, 10));
  }
};
