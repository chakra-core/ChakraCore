'use strict';

var pkg = require('../../package');

module.exports = function(app) {
  return function(val, key, config, next) {
    console.log(app.log.cyan(`${app._name} v${pkg.version}`));
    process.exit();
  };
};
