'use strict';

/**
 * Creates an index of values from two arrays to enable easy lookup.
 *
 * @private
 */
module.exports = function makeIndex(a, b) {
  const index = Object.create(null);

  ((a || []).concat(b ||[])).forEach(a => index[a] = true);

  return index;
};
