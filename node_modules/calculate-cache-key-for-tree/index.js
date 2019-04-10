'use strict';

/*
  This allows addon authors to easily utilize the "default" cache key generation
  system (e.g. if they implement `treeFor*` in a way that is totally safe), but
  still allows us to identify issues with the cache key generation and iterate
  (since both addon and ember-cli could use `^1.0.0` and float independently).
*/
module.exports = require('./lib/calculate-cache-key-for-tree');
module.exports.cacheKeyForStableTree = require('./lib/cache-key-for-stable-tree');
