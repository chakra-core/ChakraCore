'use strict';

var utils = require('../utils');

/**
 * Add a `renameKey` function to the given `app` instance.
 */

module.exports = function(config) {
  config = config || {};

  return function(app) {
    this.define('renameKey', function(key, file, fn) {
      if (typeof key === 'function') {
        return this.renameKey(null, null, key);
      }
      if (typeof file === 'function') {
        return this.renameKey(key, null, file);
      }

      if (typeof fn !== 'function') {
        fn = this.options.renameKey;
      }
      if (typeof fn !== 'function') {
        fn = config.renameKey;
      }
      if (typeof fn !== 'function') {
        fn = utils.identity;
      }
      this.options.renameKey = fn;
      if (typeof key === 'string') {
        return fn.call(this, key, file);
      }
      return this;
    });
  };
};
