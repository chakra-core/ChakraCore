'use strict';

const fs = require('fs-extra');
const path = require('path');
const quickTemp = require('quick-temp');
const Configstore = require('configstore');
const CommandGenerator = require('./command-generator');
const stableStringify = require('json-stable-stringify');
const symlinkOrCopySync = require('symlink-or-copy').sync;

let originalWorkingDirectory = process.cwd();

// Module scoped variable to store whether a particular cache has been
// attempted to be upgraded.
let upgraded = {};

/*
List of keys which could possibly result in the package manager installing
something. This is the "err on the side of caution" approach. It actually
doesn't matter if something is or isn't automatically installed in any of the
cases where we use this.
*/
let DEPENDENCY_KEYS = [
  'dependencies',
  'devDependencies',
  'peerDependencies',
  'optionalDependencies',
];

/**
 * The `bower` command helper.
 *
 * @method bower
 * @param {String} subcommand The subcommand to be passed into bower.
 * @param {String} [...arguments] Arguments to be passed into the bower subcommand.
 * @param {Object} [options={}] The options passed into child_process.spawnSync.
 *   (https://nodejs.org/api/child_process.html#child_process_child_process_spawnsync_command_args_options)
 */
let bower = new CommandGenerator('bower');

/**
 * The `npm` command helper.
 *
 * @method npm
 * @param {String} subcommand The subcommand to be passed into npm.
 * @param {String} [...arguments] Arguments to be passed into the npm subcommand.
 * @param {Object} [options={}] The options passed into child_process.spawnSync.
 *   (https://nodejs.org/api/child_process.html#child_process_child_process_spawnsync_command_args_options)
 */
let npm = new CommandGenerator('npm');

/**
 * The `yarn` command helper.
 *
 * @method yarn
 * @param {String} subcommand The subcommand to be passed into yarn.
 * @param {String} [...arguments] Arguments to be passed into the yarn subcommand.
 * @param {Object} [options={}] The options passed into child_process.spawnSync.
 *   (https://nodejs.org/api/child_process.html#child_process_child_process_spawnsync_command_args_options)
 */
let yarn = new CommandGenerator('yarn');

// This lookup exists to make it possible to look the commands up based upon context.
let originals;
let commands = {
  bower,
  npm,
  yarn,
};

// The definition list of translation terms.
let lookups = {
  manifest: {
    bower: 'bower.json',
    npm: 'package.json',
    yarn: 'package.json',
  },
  path: {
    bower: 'bower_components',
    npm: 'node_modules',
    yarn: 'node_modules',
  },
  upgrade: {
    bower: 'update',
    npm: 'install',
    yarn: 'upgrade',
  },
};

/**
 * The `translate` command is used to turn a consistent argument into
 * appropriate values based upon the context in which it is used. It's
 * a convenience helper to avoid littering lookups throughout the code.
 *
 * @method translate
 * @param {String} type Either 'bower', 'npm', or 'yarn'.
 * @param {String} lookup Either 'manifest', 'path', or 'upgrade'.
 */
function translate(type, lookup) { return lookups[lookup][type]; }

/**
 * The PackageCache wraps all package management functions. It also
 * handles initial global state setup.
 *
 * Usage:
 *
 * ```
 * let cache = new PackageCache();
 * let dir = cache.create('your-cache', 'yarn', '{
 *   "dependencies": {
 *     "lodash": "*",
 *     "ember-cli": "*"
 *   }
 * }');
 * // => process.cwd()/tmp/your-cache-A3B4C6D
 * ```
 *
 * This will generate a persistent cache which contains the results
 * of a clean installation of the `dependencies` as you specified in
 * the manifest argument. It will save the results of these in a
 * temporary folder, returned as `dir`. On a second invocation
 * (in the same process, or in a subsequent run) PackageCache will first
 * compare the manifest to the previously installed one, using the manifest
 * as the cache key, and make a decision as to the fastest way to get
 * the cache up-to-date. PackageCache guarantees that your cache will
 * always be up-to-date.
 *
 * If done in the same process, this simply returns the existing cache
 * directory with no change, making the following invocation simply a
 * cache validation check:
 *
 * ```
 * let dir2 = cache.create('your-cache', 'yarn', '{
 *   "dependencies": {
 *     "lodash": "*",
 *     "ember-cli": "*"
 *   }
 * }');
 * // => process.cwd()/tmp/your-cache-A3B4C6D
 * ```
 *
 * If you wish to modify a cache you can do so using the `update` API:
 *
 * ```
 * let dir3 = cache.update('your-cache', 'yarn', '{
 *   "dependencies": {
 *     "": "*",
 *     "lodash": "*",
 *     "ember-cli": "*"
 *   }
 * }');
 * // => process.cwd()/tmp/your-cache-A3B4C6D
 * ```
 *
 * Underneath the hood `create` and `update` are identical–which
 * makes clear the simplicity of this tool. It will always do the
 * right thing. You can think of the outcome of any `create` or
 * `update` call as identical to `rm -rf node_modules && npm install`
 * except as performant as possible.
 *
 * If you need to make modifications to a cache but wish to retain
 * the original you can invoke the `clone` command:
 *
 * ```
 * let newDir = cache.clone('your-cache', 'modified-cache');
 * let manifest = fs.readJsonSync(path.join(newDir, 'package.json'));
 * manifest.dependencies['express'] = '*';
 * cache.update('modified-cache', 'yarn', JSON.stringify(manifest));
 * // => process.cwd()/tmp/modified-cache-F8D5C8B
 * ```
 *
 * This mental model makes it easy to prevent coding mistakes, state
 * leakage across multiple test runs by making multiple caches cheap,
 * and has tremendous performance benefits.
 *
 * You can even programatically update a cache:
 *
 * ```
 * let CommandGenerator = require('./command-generator');
 * let yarn = new CommandGenerator('yarn');
 *
 * let dir = cache.create('your-cache', 'yarn', '{ ... }');
 *
 * yarn.invoke('add', 'some-addon', { cwd: dir });
 * ```
 *
 * The programmatic approach enables the entire set of usecases that
 * the underlying package manager supports while continuing to wrap it
 * in a persistent cache. You should not directly modify any files in the
 * cache other than the manifest unless you really know what you're doing as
 * that can put the cache into a possibly invalid state.
 *
 * PackageCache also supports linking. If you pass an array of module names to
 * in the fourth position it will ensure that those are linked, and remain
 * linked for the lifetime of the cache. When you link a package it uses the
 * current global link provided by the underlying package manager and invokes
 * their built-in `link` command.
 *
 * ```
 * let dir = cache.create('your-cache', 'yarn', '{ ... }', ['ember-cli']);
 * // => `yarn link ember-cli` happens along the way.
 * ```
 *
 * Sometimes this global linking behavior is not what you want. Instead you wish
 * to link in some other working directory. PackageCache makes a best effort
 * attempt at supporting this workflow by allowing you to specify an object in
 * the `links` argument array passed to `create`.
 *
 * let dir = cache.create('your-cache', 'yarn', '{ ... }', [
 *   { name: 'ember-cli', path: '/the/absolute/path/to/the/package' },
 *   'other-package'
 * ]);
 *
 * This creates a symlink at the named package path to the specified directory.
 * As package managers do different things for their own linking behavior this
 * is "best effort" only. Be sure to review upstream behavior to identify if you
 * rely on those features for your code to function:
 *
 * - [bower](https://github.com/bower/bower/blob/master/lib/commands/link.js)
 * - [npm](https://github.com/npm/npm/blob/latest/lib/link.js)
 * - [yarn](https://github.com/yarnpkg/yarn/blob/master/src/cli/commands/link.js)
 *
 * As the only caveat, PackageCache _is_ persistent. The consuming
 * code is responsible for ensuring that the cache size does not
 * grow unbounded.
 *
 * @class PackageCache
 * @constructor
 * @param {String} rootPath Root of the directory for `PackageCache`.
 */
module.exports = class PackageCache {
  constructor(rootPath) {
    this.rootPath = rootPath || originalWorkingDirectory;

    this._conf = new Configstore('package-cache');

    // Set it to where we want it to be.
    this._conf.path = path.join(this.rootPath, 'tmp', 'package-cache.json');

    // Initialize.
    this._conf.all = this._conf.all;

    this._cleanDirs();
  }

  get dirs() {
    return this._conf.all;
  }

  /**
   * The `__setupForTesting` modifies things in module scope.
   *
   * @method __setupForTesting
   */
  __setupForTesting(stubs) {
    originals = commands;
    commands = stubs.commands;
  }

  /**
   * The `__resetForTesting` puts things back in module scope.
   *
   * @method __resetForTesting
   */
  __resetForTesting() {
    commands = originals;
  }

  /**
   * The `_cleanDirs` method deals with sync issues between the
   * Configstore and what exists on disk. Non-existent directories
   * are removed from `this.dirs`.
   *
   * @method _cleanDirs
   */
  _cleanDirs() {
    let labels = Object.keys(this.dirs);

    let label, directory;
    for (let i = 0; i < labels.length; i++) {
      label = labels[i];
      directory = this.dirs[label];
      if (!fs.existsSync(directory)) {
        this._conf.delete(label);
      }
    }
  }

  /**
   * The `_readManifest` method reads the on-disk manifest for the current
   * cache and returns its value.
   *
   * @method _readManifest
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   * @return {String} The manifest file contents on disk.
   */
  _readManifest(label, type) {
    let readManifestDir = this.dirs[label];

    if (!readManifestDir) { return null; }

    let inputPath = path.join(readManifestDir, translate(type, 'manifest'));

    let result = null;
    try {
      result = fs.readFileSync(inputPath, 'utf8');
    } catch (error) {
      // Swallow non-exceptional errors.
      if (error.code !== 'ENOENT') {
        throw error;
      }
    }
    return result;
  }

  /**
   * The `_writeManifest` method generates the on-disk folder for the package cache
   * and saves the manifest into it. If it is a yarn package cache it will remove
   * the existing lock file.
   *
   * @method _writeManifest
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   * @param {String} manifest The contents of the manifest file to write to disk.
   */
  _writeManifest(label, type, manifest) {
    process.chdir(this.rootPath);
    let outputDir = quickTemp.makeOrReuse(this.dirs, label);
    process.chdir(originalWorkingDirectory);

    this._conf.set(label, outputDir);

    let outputFile = path.join(outputDir, translate(type, 'manifest'));
    fs.outputFileSync(outputFile, manifest);

    // Remove any existing yarn.lock file so that it doesn't try to incorrectly use it as a base.
    if (type === 'yarn') {
      try {
        fs.unlinkSync(path.join(outputDir, 'yarn.lock'));
      } catch (error) {
        // Catch unexceptional error but rethrow if something is truly wrong.
        if (error.code !== 'ENOENT') {
          throw error;
        }
      }
    }
  }

  /**
   * The `_removeLinks` method removes from the dependencies of the manifest the
   * assets which will be linked in so that we don't duplicate install. It saves
   * off the values in the internal `PackageCache` metadata for restoration after
   * linking as those values may be necessary.
   *
   * It is also responsible for removing these links prior to making any changes
   * to the specified cache.
   *
   * @method _removeLinks
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   */
  _removeLinks(label, type) {
    let cachedManifest = this._readManifest(label, type);
    if (!cachedManifest) { return; }

    let jsonManifest = JSON.parse(cachedManifest);
    let links = jsonManifest._packageCache.links;

    // Blindly remove existing links whether or not they appear in the manifest.
    let link, linkPath;
    for (let i = 0; i < links.length; i++) {
      link = links[i];
      if (typeof link === 'string') {
        commands[type].invoke('unlink', link, { cwd: this.dirs[label] });
      } else {
        linkPath = path.join(this.dirs[label], translate(type, 'path'), link.name);
        try {
          fs.removeSync(linkPath);
        } catch (error) {
          // Catch unexceptional error but rethrow if something is truly wrong.
          if (error.code !== 'ENOENT') {
            throw error;
          }
        }

      }
    }

    // Remove things from the manifest which we know we'll link back in.
    let originals = {};
    let key, linkName;
    for (let i = 0; i < DEPENDENCY_KEYS.length; i++) {
      key = DEPENDENCY_KEYS[i];
      if (jsonManifest[key]) {
        // Get a clone of the original object.
        originals[key] = JSON.parse(JSON.stringify(jsonManifest[key]));
      }
      for (let j = 0; j < links.length; j++) {
        link = links[j];

        // Support object-style invocation for "manual" linking.
        if (typeof link === 'string') {
          linkName = link;
        } else {
          linkName = link.name;
        }

        if (jsonManifest[key] && jsonManifest[key][linkName]) {
          delete jsonManifest[key][linkName];
        }
      }
    }

    jsonManifest._packageCache.originals = originals;
    let manifest = JSON.stringify(jsonManifest);

    this._writeManifest(label, type, manifest);
  }

  /**
   * The `_restoreLinks` method restores the dependencies from the internal
   * `PackageCache` metadata so that the manifest matches its original state after
   * performing the links.
   *
   * It is also responsible for restoring these links into the `PackageCache`.
   *
   * @method _restoreLinks
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   */
  _restoreLinks(label, type) {
    let cachedManifest = this._readManifest(label, type);
    if (!cachedManifest) { return; }

    let jsonManifest = JSON.parse(cachedManifest);
    let links = jsonManifest._packageCache.links;

    // Blindly restore links.
    let link, linkPath;
    for (let i = 0; i < links.length; i++) {
      link = links[i];
      if (typeof link === 'string') {
        commands[type].invoke('link', link, { cwd: this.dirs[label] });
      } else {
        linkPath = path.join(this.dirs[label], translate(type, 'path'), link.name);
        fs.mkdirsSync(path.dirname(linkPath)); // Just in case the path doesn't exist.
        symlinkOrCopySync(link.path, linkPath);
      }
    }

    // Restore to original state.
    let keys = Object.keys(jsonManifest._packageCache.originals);
    let key;
    for (let i = 0; i < keys.length; i++) {
      key = keys[i];
      jsonManifest[key] = jsonManifest._packageCache.originals[key];
    }

    // Get rid of the originals.
    delete jsonManifest._packageCache.originals;

    // Serialize back to disk.
    let manifest = JSON.stringify(jsonManifest);
    this._writeManifest(label, type, manifest);
  }

  /**
   * The `_checkManifest` method compares the desired manifest to that which
   * exists in the cache.
   *
   * @method _checkManifest
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   * @param {String} manifest The contents of the manifest file to compare to cache.
   * @return {Boolean} `true` if identical.
   */
  _checkManifest(label, type, manifest) {
    let cachedManifest = this._readManifest(label, type);

    if (cachedManifest === null) { return false; }

    let parsedCached = JSON.parse(cachedManifest);
    let parsedNew = JSON.parse(manifest);

    // Only inspect the keys we care about.
    // Invalidate the cache based off the private _packageCache key as well.
    let keys = [].concat(DEPENDENCY_KEYS, '_packageCache');

    let key, before, after;
    for (let i = 0; i < keys.length; i++) {
      key = keys[i];

      // Empty keys are identical to undefined keys.
      before = stableStringify(parsedCached[key]) || '{}';
      after = stableStringify(parsedNew[key]) || '{}';

      if (before !== after) {
        return false;
      }
    }

    return true;
  }

  /**
   * The `_install` method installs the contents of the manifest into the
   * specified package cache.
   *
   * @method _install
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   */
  _install(label, type) {
    this._removeLinks(label, type);
    commands[type].invoke('install', { cwd: this.dirs[label] });
    this._restoreLinks(label, type);

    // If we just did a clean install we can treat it as up-to-date.
    upgraded[label] = true;
  }

  /**
   * The `_upgrade` method guarantees that the contents of the manifest are
   * allowed to drift in a SemVer compatible manner. It ensures that CI is
   * always running against the latest versions of all dependencies.
   *
   * @method _upgrade
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   */
  _upgrade(label, type) {
    // Lock out upgrade calls after the first time upgrading the cache.
    if (upgraded[label]) { return; }

    // Only way to get repeatable behavior in npm: start over.
    // We turn an `_upgrade` task into an `_install` task.
    if (type === 'npm') {
      fs.removeSync(path.join(this.dirs[label], translate(type, 'path')));
      return this._install(label, type);
    }

    this._removeLinks(label, type);
    commands[type].invoke(translate(type, 'upgrade'), { cwd: this.dirs[label] });
    this._restoreLinks(label, type);

    upgraded[label] = true;
  }

  // PUBLIC API BELOW

  /**
   * The `create` method adds a new package cache entry.
   *
   * @method create
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   * @param {String} manifest The contents of the manifest file for the cache.
   * @param {Array} links Packages to omit for install and link.
   * @return {String} The directory on disk which contains the cache.
   */
  create(label, type, manifest, links) {
    links = links || [];

    // Save metadata about the PackageCache invocation in the manifest.
    let packageManagerVersion = commands[type].invoke('--version').stdout;

    let jsonManifest = JSON.parse(manifest);
    jsonManifest._packageCache = {
      node: process.version,
      packageManager: type,
      packageManagerVersion,
      links,
    };

    manifest = JSON.stringify(jsonManifest);

    // Compare any existing manifest to the ideal per current blueprint.
    let identical = this._checkManifest(label, type, manifest);

    if (identical) {
      // Use what we have, but opt in to SemVer drift.
      this._upgrade(label, type);
    } else {
      // Tell the package manager to start semi-fresh.
      this._writeManifest(label, type, manifest);
      this._install(label, type);
    }

    return this.dirs[label];
  }

  /**
   * The `update` method aliases the `create` method.
   *
   * @method update
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   * @param {String} manifest The contents of the manifest file for the cache.
   * @param {Array} links Packages to elide for install and link.
   * @return {String} The directory on disk which contains the cache.
   */
  update(/*label, type, manifest, links*/) {
    return this.create.apply(this, arguments);
  }

  /**
   * The `get` method returns the directory for the cache.
   *
   * @method get
   * @param {String} label The label for the cache.
   * @return {String} The directory on disk which contains the cache.
   */
  get(label) {
    return this.dirs[label];
  }

  /**
   * The `destroy` method removes all evidence of the package cache.
   *
   * @method destroy
   * @param {String} label The label for the cache.
   * @param {String} type The type of package cache.
   */
  destroy(label) {
    process.chdir(this.rootPath);
    quickTemp.remove(this.dirs, label);
    process.chdir(originalWorkingDirectory);

    this._conf.delete(label);
  }

  /**
   * The `clone` method duplicates a cache. Some package managers can
   * leverage a pre-existing state to speed up their installation.
   *
   * @method destroy
   * @param {String} fromLabel The label for the cache to clone.
   * @param {String} toLabel The label for the new cache.
   */
  clone(fromLabel, toLabel) {
    process.chdir(this.rootPath);
    let outputDir = quickTemp.makeOrReuse(this.dirs, toLabel);
    process.chdir(originalWorkingDirectory);

    this._conf.set(toLabel, outputDir);

    fs.copySync(this.get(fromLabel), outputDir);

    return this.dirs[toLabel];
  }
};
