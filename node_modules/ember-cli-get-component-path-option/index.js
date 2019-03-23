'use strict';

module.exports = function getPathOption(options) {
  var outputPath = 'components';
  if (options === null || typeof options !== 'object') {
    throw new TypeError('getPathOptions first argument must be an object');
  }
  if (options.path) {
    outputPath = options.path;
  } else {
    if (options.path === '') {
      outputPath = '';
    }
  }
  return outputPath;
};
