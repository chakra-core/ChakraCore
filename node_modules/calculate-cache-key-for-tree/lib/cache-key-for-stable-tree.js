'use strict';

const calculateCacheKeyForTree = require('./calculate-cache-key-for-tree');

/*
  An implementation of Addon#cacheKeyForTree that assumes the return values of
  all trees is stable. This makes it easy to opt-back-in to tree caching for
  addons that implement stable overrides of treeFor hooks.

  @public
  @method cacheKeyForStableTree
  @param {String} treeType
  @return {String} cacheKey
*/
module.exports = function cacheKeyForStableTree(treeType) {
  let addon = this;
  return calculateCacheKeyForTree(treeType, addon);
};
