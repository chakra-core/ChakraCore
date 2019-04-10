'use strict';

var utils = require('./utils');

module.exports = function(config, schema) {
  if (schema.isMerged) return;

  schema.checked = schema.checked || {};
  schema.isMerged = true;

  // shallow clone schema.data
  var data = utils.extend({}, schema.data);
  var obj = utils.merge({}, config, data);
  utils.merge(config, obj);
};
