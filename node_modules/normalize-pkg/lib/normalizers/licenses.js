'use strict';

var license = require('./license');
var merge = require('../merge');

module.exports = function(val, key, config, schema) {
  merge(config, schema);
  if (Array.isArray(val)) {
    schema.warning('deprecated', key, {actual: key, expected: 'license'});
    schema.update('license', val[0].type, config);
    delete config[key];
  } else {
    delete config[key];
    return license(val, 'license', config, schema);
  }
};
