'use strict';
var crypto = require('crypto');

/*
 * Create a cache key for the given name and dir. This is used for all the
 * keys in the various caches within hash-for-dep.
 *
 * @param name is the 'require'-style name (i.e., could include a full path,
 * or be relative to dir)
 * @param dir optional directory to start resolution from when running
 * the node-resolution algorithm as 'require' does.
 */
module.exports = function cacheKey(name, dir) {
  return crypto.createHash('sha1').update(name + '\x00' + dir).digest('hex');
};
