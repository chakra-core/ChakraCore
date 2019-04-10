'use strict';

module.exports = function(val, key, config, schema) {
  schema.update('bin', config);
  if (!config.hasOwnProperty('bin') && val === true) {
    schema.warning('custom', key, 'expected "bin" to be defined when "preferGlobal" is true');
  }
  return val === true;
};
