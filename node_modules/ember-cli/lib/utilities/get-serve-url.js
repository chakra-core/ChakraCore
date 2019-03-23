'use strict';

const cleanBaseURL = require('clean-base-url');

module.exports = function(options, project) {
  let config = project.config(options.environment);

  let baseURL = config.rootURL === '' ? '/' : cleanBaseURL(config.rootURL || (config.baseURL || '/'));
  return `http${options.ssl ? 's' : ''}://${options.host || 'localhost'}:${options.port}${baseURL}`;
};
