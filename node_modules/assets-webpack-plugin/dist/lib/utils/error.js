'use strict';

var _ = require('lodash');

module.exports = function pluginError(message, previousError) {
  var err = new Error('[AssetsWebpackPlugin] ' + message);

  return previousError ? _.assign(err, previousError) : err;
};