'use strict';

const IS_GLOB = /[\{\}\*\[\]]/;

/**
 * Ensures the provided list of files does not contain any glob expressions.
 *
 * @private
 */
module.exports = function ensureNoGlob(listName, list) {
  (list || []).forEach(a => {
    if (IS_GLOB.test(a)) {
      throw new TypeError(listName + ' cannot contain a glob,  `' + a + '`');
    }
  });
};
