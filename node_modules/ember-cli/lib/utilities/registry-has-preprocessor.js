'use strict';

module.exports = function registryHasPreprocessor(registry, type) {
  return registry.load(type).length > 0;
};
