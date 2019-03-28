'use strict';

module.exports = function(val, key, config, schema) {
  if (typeof val === 'boolean') {
    schema.warning('deprecated', key, {actual: key, expected: 'engine-strict'});
    config['engine-strict'] = val;
  }
  schema.omit(key);
};
