'use strict';

var Cache = require('./cache');

/*
 * CacheGroup is used to both speed up and ensure consistency of hashForDep.
 *
 * The CacheGroup contains three separate caches:
 *
 *  - MODULE_ENTRY: the cache of realPathKey => ModuleEntry objects. 
 *    Each ModuleEntry contains information about a particular module 
 *    found during hash-for-dep processing.  realPathKey is a hash
 *    of the resolved real path to the module's package.json file.
 * 
 *    Having the real path means that when resolving dependencies, we can
 *    take the resolved paths, hash them and check quickly for cache entries,
 *    speeding creation of the cache. Because many modules may refer to
 *    the same module as a dependency, this eliminates duplication and having
 *    to reread package.json files.
 *
 *    However, that also means we need a second cache to map from the original
 *    name and dir passed to hashForDep to the final resolved path for the
 *    module's package.json file.
 *
 *  - PATH: the cache of nameDirKey => realPathKey. nameDirKey is a hash of the
 *    combination of the name and starting directory passed to hashForDep.
 *    realPathKey is a hash of the resulting real path to the relevant package.json file.
 *
 *  - REAL_FILE_PATH: the cache of filePath => resolved realPath for relevant files.
 *    When determining the location of a file, the file path may involve links.
 *    This cache is keyed on the original file path, with a value of the real path.
 *    This cache helps to speed up path resolution in both cases by minimizing 
 *    the use of costly 'fs.statSync' calls.
 *
 *  - REAL_DIRECTORY_PATH: the cache of filePath => resolved realPath for relevant directories.
 *    When determining the location of a file by going up the node_modules chain, 
 *    paths to intervening directories may contain links. Similar to REAL_FILE_PATH,
 *    this cache is keyed on the original directory path, with a value of the real path.
 *    This cache helps to speed up path resolution in both cases by minimizing 
 *    the use of costly 'fs.statSync' calls.
 * 
 * Note: when discussing 'real paths' above, the paths are normalized by routines
 * like path.join() or path.resolve(). We do nothing beyond that (e.g., we do not attempt
 * to force the paths into POSIX style.)
 */

module.exports = function CacheGroup() {
  this.MODULE_ENTRY = new Cache();
  this.PATH = new Cache();
  this.REAL_FILE_PATH = new Cache();
  this.REAL_DIRECTORY_PATH = new Cache();
  Object.freeze(this);
};
