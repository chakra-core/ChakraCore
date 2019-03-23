'use strict';

const crypto = require('crypto');
const stringify = require('json-stable-stringify');

/*
  Calculates the cache key for a given type of tree (e.g., addon, app, etc.) for
  a given instance of an Addon. By default, uses the addon's package.json and
  name to generate the cache key.

  @public
  @method calculateCacheKeyForTree
  @param {String} treeType
  @param {Addon} addonInstance
  @param {Array<any>} additionalCacheKeyParts
  @return {String} cacheKey
*/
module.exports = function calculateCacheKeyForTree(treeType, addonInstance, additionalCacheKeyParts) {
  let cacheKeyParts = [
    addonInstance.pkg,
    addonInstance.name,
    treeType
  ].concat(additionalCacheKeyParts);

  return crypto.createHash('md5').update(stringify(cacheKeyParts), 'utf8').digest('hex');
};
