'use strict';

const Concat = require('./concat');
const merge = require('lodash.merge');

module.exports = function(inputNode, options) {
  if (!options || !options.outputFile) {
    throw new Error('outputFile is required');
  }

  let config = merge({
    enabled: true
  }, options.sourceMapConfig);

  let Strategy;

  if (config.enabled) {
    let extensions = (config.extensions || ['js']);
    for (let i=0; i < extensions.length; i++) {
      let ext = '.' + extensions[i].replace(/^\./,'');
      if (options.outputFile.slice(-1 * ext.length) === ext) {
        Strategy = require('fast-sourcemap-concat');
        break;
      }
    }
  }

  return new Concat(inputNode, options, Strategy || require('./lib/strategies/simple'));
};
