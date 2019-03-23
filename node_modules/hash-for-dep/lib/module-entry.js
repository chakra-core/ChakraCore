'use strict';

var resolvePackagePath = require('resolve-package-path');
var cacheKey = require('./cache-key');
var tryRequire = require('./try-require');
var crypto = require('crypto');

/*
 * Entries in 'MODULE_ENTRY' cache. The idea is to minimize the information needed for
 * each entry (don't store the entire package.json object), but keep
 * information around for all parts of computing a hash for the dependency
 * tree of the original module named by 'name' and 'dir' in hashForDep.
 * The ModuleEntry is created at the time the package.json file is
 * read in (so that's only done once), but some properties may not be initialized
 * until later (this then supports creating cycles in the dependencies list,
 * which is supported by Node).
 */
module.exports = ModuleEntry;

function ModuleEntry(name, version, rootDir, sourceHash, isValid) {

  // The name of the module, from its package.json file.
  this.name = name;

  // The version of the module, from its package.json file.
  this.version = version;

  // The resolved real file path for the directory containing the package.json file.
  // The string has no trailing path separator(s), as path.resolve() removes them.
  this.rootDir = rootDir;

  // The computed hash for only the module's source files (under
  // the root dir, but specifically NOT any node_modules dependencies).
  // See locate() for initialization of this value.
  this._sourceHash = sourceHash;

  // The computed hash for the module's source files plus
  // the source hashes of the dependencies, recursively to the
  // leaves of the dependency tree.
  this._hash = null;

  // References to other ModuleEntry objects that correspond
  // to the dependencies listed in the package.json file.
  // See locate() for initialization of this value.
  this._dependencies = Object.create(null);

  this.isValid = isValid === false ? false : true;
}

ModuleEntry.prototype.addDependency = function(dependency, moduleEntry) {
  this._dependencies[dependency] = moduleEntry;
};

/*
 * Starting from an initial ModuleEntry, traverse the entry's dependencies
 * ("almost" ModuleEntry objects) recursively to the leaves of the dependency
 * tree. Return an object containing all the ModuleEntry objects,
 * keyed by each entry's rootDir (real path to the module's root directory.)
 * NOTE: for speed, the initial caller of _gatherDependencies must pass
 * in a result object, so we aren't constantly checking that it is set.
 */
ModuleEntry.prototype._gatherDependencies = function(dependencies) {

  var moduleEntry = this;

  if (dependencies[moduleEntry.rootDir] !== undefined) {
    // we already hit this path during the dependencies somewhere earlier
    // in the dependency tree, so avoid cycling.
    return;
  }

  dependencies[moduleEntry.rootDir] = moduleEntry;

  Object.keys(moduleEntry._dependencies).forEach(function(dep) {
    moduleEntry._dependencies[dep]._gatherDependencies(dependencies);
  });

  return dependencies;
};

/*
 * Compute/return the 'hash' field for the given moduleEntry.
 * The hash actually consists only of the _sourceHash values of all the
 * dependencies (ModuleEntry objects) concatenated with '\x00' between
 * entries. The '_sourceHash' values are computed at the time the
 * ModuleEntry is created, so don't need to be done again.
 */
ModuleEntry.prototype.getHash = function(heimdallNode) {
  if (this._hash !== null) {
    return this._hash;
  }

  // We haven't computed the hash for this entry yet, though the
  // ModuleEntry entries are all present. It is possible that we
  // have the hash for some dependencies already.

  // Compute the full transitive dependency list. There are no duplicates,
  // because we check during _gatherDependencies before inserting in the result.
  // The keys of the returned object are the package.json baseDir values (the
  // moduleEntry.rootDir). The values are the moduleEntry objects.
  var dependencies = this._gatherDependencies(Object.create(null));

  // For repeatability between runs, sort the dependencies list keys
  // (rootDir values) before assembling into a final hash value.
  var dependencyRootDirs = Object.keys(dependencies).sort();

  if (heimdallNode) {
    heimdallNode.stats.paths += dependencyRootDirs.length;
  }

  var sourceHashes = dependencyRootDirs.map(function(rootDir) {
    return dependencies[rootDir]._sourceHash;
  }).join('\x00');

  var hash = crypto.createHash('sha1').update(sourceHashes).digest('hex');

  return (this._hash = hash);
};

/*
 * This exists to maintain compatibbility with some unexpected old behavior.
 * Specifically invalid input, would still produce a hash.
 *
 * This is clearly not helpful, but to fix a regression we will restore this
 * behavior, but the next major version (2.x.x) we should remove support for
 * this, and instead error with a helpful error message
 */
ModuleEntry.buildInvalidModule = function(name, dir) {
  return new this(name, '0.0.0' , dir, crypto.createHash('sha1').update([name, dir].filter(Boolean).join('\x00')).digest('hex'), false);
};

/*
 * Compute and return the ModuleEntry for a given name/dir pair. This is done
 * recursively so a whole tree can be computed all at once, independent of the
 * hashing functions. We also compute the '_sourceHash' value (i.e. the hash of
 * just the package source files, excluding the node_modules dependencies) at
 * the same time to make it easier to establish the 'hash' value (includes
 * dependency _sourceHash values) later.
 * Note this is a class function, not an instance function.
 */
ModuleEntry.locate = function(caches, name, dir, hashTreeFn) {
  var Constructor = this;

  var nameDirKey = cacheKey(name, dir);
  var realPathKey;

  // It's possible that for a given name/dir pair, there is no package.
  // Record a null entry in CACHES.PATH anyway so we don't need to
  // redo that search each time.
  if (caches.PATH.has(nameDirKey)) {
    realPathKey = caches.PATH.get(nameDirKey);
    if (realPathKey !== null) {
      return caches.MODULE_ENTRY.get(realPathKey);
    } else {
      return this.buildInvalidModule(name, dir);
    }
  }

  // There is no caches.PATH entry. Try to get a real package.json path. If there
  // isn't a real path, the name+dir reference is invalid, so note that in caches.PATH.
  // If there is a real path, check if it is already in the caches.MODULE_ENTRY.
  // If not, create it and insert it.
  var realPath = resolvePackagePath(name, dir, caches);;

  if (realPath === null) {
      return this.buildInvalidModule(name, dir);
  }

  // We have a path to a file that supposedly is package.json. We need to be sure
  // that either we already have a caches.MODULE_ENTRY entry (in which case we can
  // just create a caches.PATH entry to point to it) or that we can read the
  // package.json file, at which point we can create a caches.MODULE_ENTRY entry,
  // then finally the new caches.PATH ENTRY.

  // Generate the cache key for a given real file path. This key is then
  // used as the key for entries in the CacheGroup.MODULE_ENTRY cache
  // and values in the CacheGroup.PATH cache.
  realPathKey = cacheKey('hashed:' + realPath, '');

  if (caches.MODULE_ENTRY.has(realPathKey)) {
    caches.PATH.set(nameDirKey, realPathKey);
    return caches.MODULE_ENTRY.get(realPathKey);
  }

  // Require the package.  If we get a 'module-not-found' error it will return null
  // and we can insert an entry in the cache saying we couldn't find the package
  // in case it's requested later. Any other more serious error we specifically
  // don't try to catch here.
  var thePackage = tryRequire(realPath);

  // if the package was not found, do as above to create a caches.PATH entry
  // that refers to no path, so we don't waste time doing it again later.
  if (thePackage === null) {
    caches.PATH.set(nameDirKey, null);
    return thePackage;
  }

  // We have the package object and the relevant keys.

  // Compute the dir containing the package.json,
  var rootDir = realPath.slice(0, realPath.length - 13);  // length('/package.json') === 13

  // Create and insert the new ModuleEntry into the cache.MODULE_ENTRY
  // now so we know to stop when dealing with cycles.
  var moduleEntry = new Constructor(thePackage.name, thePackage.version, rootDir, hashTreeFn(rootDir));

  caches.MODULE_ENTRY.set(realPathKey, moduleEntry);
  caches.PATH.set(nameDirKey, realPathKey);

  // compute the dependencies here so the ModuleEntry doesn't need to know
  // about caches or the hashTreeFn or the package and we don't need to
  // guard against accidental reinitialization of dependencies.
  if (thePackage.dependencies) {
    // Recursively locate the references to other moduleEntry objects for the module's
    // dependencies. Initially we just do the 'local' dependencies (i.e. the ones referenced
    // in the package.json's 'dependencies' field.) Later, moduleEntry._gatherDependencies
    // is called to compute the complete list including transitive dependencies, and that
    // is then used for the final moduleEntry._hash calculation.
    Object.keys(thePackage.dependencies).sort().forEach(function(dep) {
      var dependencyModuleEntry = Constructor.locate(caches, dep, rootDir, hashTreeFn);

      // there's not really a good reason to include a failed resolution
      // of a package in the dependencies list.
      if (dependencyModuleEntry !== null) {
        moduleEntry.addDependency(dep, dependencyModuleEntry);
      }
    });
  }

  return moduleEntry;
};
