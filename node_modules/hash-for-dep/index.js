'use strict';

var hashTree = require('./lib/hash-tree');
var heimdall = require('heimdalljs');
var CacheGroup = require('./lib/cache-group');
var cacheKey = require('./lib/cache-key');
var logger = require('heimdalljs-logger')('hash-for-dep:');
var ModuleEntry = require('./lib/module-entry');

/*
 * hash-for-dep by default operates by caching all its findings until the
 * current process exits.
 *
 * In some less common scenarios, it may be appropriate to skip this process
 * cache. To do so, hashForDep can be invoked with the fourth argument = true.
 *
 * ```
 * hashForDep(name, dir, null, true);
 * ```
 *
 * Doing so will not interfere with the existing process cache. Instead, 
 * the current invocation of hashForDep is given its own unique cache.
 * This cache is used to prevent duplicate work within that invocation of
 * hashForDep, and is discarded once the invocation completes.
 */

/*
 * PROCESS_CACHE is the default cache used until the process exits.
 */
var PROCESS_CACHE = new CacheGroup();

function HashForDepSchema() {
  this.paths = 0;
}


/* @public
 *
 * @method hashForDep
 * @param {String} name name of the dependency module.
 * @param {String} dir (optional) root dir to run the hash resolving from
 * @param {String} _hashTreeOverride (optional) private, used internally for testing
 * @param {Boolean} _skipCache (optional) intended to bypass cache
 * @return {String} a hash representing the stats of this module and all its descendents
 */
module.exports = function hashForDep(name, dir, _hashTreeOverride, _skipCache) {

  var skipCache = (typeof _hashTreeOverride === 'function' || _skipCache === true);

  var caches = skipCache ? new CacheGroup() : PROCESS_CACHE;

  var hashTreeFn = (_hashTreeOverride || hashTree);

  var heimdallNodeOptions = {
    name: 'hashForDep(' + name + ')',
    hashForDep: true,
    dependencyName: name,
    rootDir: dir,
    skipCache: skipCache,
    cacheKey: cacheKey(name, dir)
  };

  var h = heimdall.start(heimdallNodeOptions, HashForDepSchema);

  try {
    var moduleEntry = ModuleEntry.locate(caches, name, dir, hashTreeFn);
    return moduleEntry.getHash(h);
  } finally {
    h.stop();
  }
};

module.exports._resetCache = function() {
  PROCESS_CACHE = new CacheGroup();
};

// Expose the module entry cache for testing only
Object.defineProperty(module.exports, '_cache', {
  get: function() {
    return PROCESS_CACHE.MODULE_ENTRY;
  }
});


// Expose the process cache for testing only
Object.defineProperty(module.exports, '_caches', {
  get: function() {
    return PROCESS_CACHE;
  }
});
