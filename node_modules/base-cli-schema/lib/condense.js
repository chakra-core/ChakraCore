'use strict';

var utils = require('./utils');
var fields = require('./config/');
var debug = require('debug')('base:cli');

/**
 * Normalize the config object to be written to either `app.store.data`
 * or the user's local config. Local config is usually a property on
 * package.json, but may also be a separate file.
 */

module.exports = function(app, val, existing) {
  var opts = {omitEmpty: true, sortArrays: false};

  opts.keys = ['init', 'run', 'toc', 'layout', 'tasks', 'options', 'data', 'plugins', 'helpers', 'related', 'reflinks'];

  var schema = new utils.Schema(opts);
  for (var key in fields) {
    debug('adding schema field > %s', key);
    schema.field(key, fields[key](app, existing));
  }
  return schema.normalize(val);
};
