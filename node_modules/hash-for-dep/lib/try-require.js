'use strict';

/*
 * attempt to require a given node_module.
 * If the module was found, return found value
 * If the module was NOT found, return null
 * If requiring the module crashes for an unexpected reason, throw the unexpected error
 */
module.exports = function tryRequire(moduleName) {
  try {
    return require(moduleName);
  } catch (err) {
    if (err !== null && typeof err === 'object' && err.code === 'MODULE_NOT_FOUND') {
      return null;
    } else {
      throw err;
    }
  }
};
