'use strict';

var pathTemplate = require('./pathTemplate');

module.exports = function isSourceMap(options, asset) {
  var sourceMapFilename = options.output.sourceMapFilename;
  var sourcemapTemplate = pathTemplate(sourceMapFilename);
  return sourcemapTemplate.matches(asset);
};