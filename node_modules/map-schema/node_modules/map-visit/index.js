'use strict';

var utils = require('./utils');

/**
 * Map `visit` over an array of objects.
 *
 * @param  {Object} `collection` The context in which to invoke `method`
 * @param  {String} `method` Name of the method to call on `collection`
 * @param  {Object} `arr` Array of objects.
 */

module.exports = function mapVisit(collection, method, arr) {
  if (!Array.isArray(arr)) {
    throw new TypeError('expected an array');
  }

  arr.forEach(function(val) {
    if (typeof val === 'string') {
      collection[method](val);
    } else {
      utils.visit(collection, method, val);
    }
  });
};
