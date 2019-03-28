'use strict';

var pathTemplate = require('./pathTemplate');

module.exports = function isHMRUpdate(options, asset) {
  var hotUpdateChunkFilename = options.output.hotUpdateChunkFilename;
  var hotUpdateTemplate = pathTemplate(hotUpdateChunkFilename);
  return hotUpdateTemplate.matches(asset);
};