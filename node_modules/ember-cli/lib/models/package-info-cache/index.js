'use strict';

/*
 * Performance cache for information about packages (projects/addons/"apps"/modules)
 * under an initial root directory and resolving addon/dependency links to other packages.
 */

const fs = require('fs-extra');
const path = require('path');
const ErrorList = require('./error-list');
const Errors = require('./errors');
const PackageInfo = require('./package-info');
const NodeModulesList = require('./node-modules-list');
const logger = require('heimdalljs-logger')('ember-cli:package-info-cache');

let realFilePathCache;
let realDirectoryPathCache;

/**
 * Resolve the real path for a file, return null if does not
 * exist or is not a file or FIFO, return the real path otherwise.
 *
 * @private
 * @method getRealFilePath
 * @param  {String} filePath the path to the file of interest
 * @return {String} real path or null
 */
function getRealFilePath(filePath) {
  let realPath;

  try {
    realPath = realFilePathCache[filePath];

    if (realPath) {
      return realPath;
    }

    let stat = fs.statSync(filePath);

    if (stat.isFile() || stat.isFIFO()) {
      realPath = fs.realpathSync(filePath);
    }
  } catch (e) {
    if (
      e !== null &&
      typeof e === 'object' &&
      (e.code === 'ENOENT' || e.code === 'ENOTDIR')
    ) {
      realPath = null;
    } else {
      throw e;
    }
  }

  realFilePathCache[filePath] = realPath;

  return realPath;
}

/**
 * Resolve the real path for a directory, return null if does not
 * exist or is not a directory, return the real path otherwise.
 *
 * @private
 * @method getRealDirectoryPath
 * @param  {String} directoryPath the path to the directory of interest
 * @return {String} real path or null
 */
function getRealDirectoryPath(directoryPath) {
  let realPath;

  try {
    realPath = realDirectoryPathCache[directoryPath];

    if (realPath) {
      return realPath;
    }

    let stat = fs.statSync(directoryPath);

    if (stat.isDirectory()) {
      realPath = fs.realpathSync(directoryPath);
    }
  } catch (e) {
    if (
      e !== null &&
      typeof e === 'object' &&
      (e.code === 'ENOENT' || e.code === 'ENOTDIR')
    ) {
      realPath = null;
    } else {
      throw e;
    }
  }

  realDirectoryPathCache[directoryPath] = realPath;

  return realPath;
}

const PACKAGE_JSON = 'package.json';

/**
 * Class that stores entries that are either PackageInfo or NodeModulesList objects.
 * The entries are stored in a map keyed by real directory path.
 *
 * @public
 * @class PackageInfoCache
 */
class PackageInfoCache {
  constructor(ui) {
    this.ui = ui; // a console-ui instance
    this._clear();
  }

  /**
   * Clear the cache information.
   *
   * @private
   * @method _clear
   */
  _clear() {
    this.entries = Object.create(null);
    this.projects = [];
    realFilePathCache = Object.create(null);
    realDirectoryPathCache = Object.create(null);
  }

  /**
   * Indicates if there is at least one error in any object in the cache.
   *
   * @public
   * @method hasErrors
   * @return true if there are any errors in the cache, for any entries, else false.
   */
  hasErrors() {
    let paths = Object.keys(this.entries);

    if (paths.find(entryPath => this.getEntry(entryPath).hasErrors())) {
      return true;
    }

    return false;
  }

  /**
   * Gather all the errors in the PIC and any cached objects, then dump them
   * out to the ui-console.
   *
   * @public
   * @method showErrors
   */
  showErrors() {
    let paths = Object.keys(this.entries).sort();

    paths.forEach(entryPath => {
      this._showObjErrors(this.getEntry(entryPath));
    });
  }

  /**
   * Dump all the errors for a single object in the cache out to the ui-console.
   *
   * Special case: because package-info-cache also creates PackageInfo objects for entries
   * that do not actually exist (to allow simplifying the code), if there's a case where
   * an object has only the single error ERROR_PACKAGE_DIR_MISSING, do not print
   * anything. The package will have been found as a reference from some other
   * addon or the root project, and we'll print a reference error there. Having
   * both is just confusing to users.
   *
   * @private
   * @method _showObjErrors
   */
  _showObjErrors(obj) {
    let errorEntries = (obj.hasErrors() ? obj.errors.getErrors() : null);

    if (!errorEntries ||
        (errorEntries.length === 1 && errorEntries[0].type === Errors.ERROR_PACKAGE_DIR_MISSING)) {
      return;
    }

    logger.info('');
    let rootPath;

    if (obj instanceof PackageInfoCache) {
      logger.info('Top level errors:');
      rootPath = this.realPath || '';
    } else {
      let typeName = (obj.project ? 'project' : 'addon');

      logger.info(`The 'package.json' file for the ${typeName} at ${obj.realPath}`);
      rootPath = obj.realPath;
    }

    errorEntries.forEach(errorEntry => {
      switch (errorEntry.type) {
        case Errors.ERROR_PACKAGE_JSON_MISSING:
          logger.info(`  does not exist`);
          break;
        case Errors.ERROR_PACKAGE_JSON_PARSE:
          logger.info(`  could not be parsed`);
          break;
        case Errors.ERROR_EMBER_ADDON_MAIN_MISSING:
          logger.info(
            `  specifies a missing ember-addon 'main' file at relative path '${path.relative(
              rootPath,
              errorEntry.data
            )}'`
          );
          break;
        case Errors.ERROR_DEPENDENCIES_MISSING:
          if (errorEntry.data.length === 1) {
            logger.info(
              `  specifies a missing dependency '${errorEntry.data[0]}'`
            );
          } else {
            logger.info(`  specifies some missing dependencies:`);
            errorEntry.data.forEach(dependencyName => {
              logger.info(`    ${dependencyName}`);
            });
          }
          break;
        case Errors.ERROR_NODEMODULES_ENTRY_MISSING:
          logger.info(`  specifies a missing 'node_modules/${errorEntry.data}' directory`);
          break;
      }
    });
  }

  /**
   * Process the root directory of a project, given a
   * Project object (we need the object in order to find the internal addons).
   * _readPackage takes care of the general processing of the root directory
   * and common locations for addons, filling the cache with each. Once it
   * returns, we take care of the locations for addons that are specific to
   * projects, not other packages (e.g. internal addons, cli root).
   *
   * Once all the project processing is done, go back through all cache entries
   * to create references between the packageInfo objects.
   *
   * @public
   * @method loadProject
   * @param projectInstance the instance of the Project object to load package data
   * about into the cache.
   * @return {PackageInfo} the PackageInfo object for the given Project object.
   * Note that if the project path is already in the cache, that will be returned.
   * No copy is made.
   */
  loadProject(projectInstance) {

    let pkgInfo = this._readPackage(projectInstance.root, projectInstance.pkg, true);

    // NOTE: the returned val may contain errors, or may contain
    // other packages that have errors. We will try to process
    // things anyway.
    if (!pkgInfo.processed) {
      this.projects.push(projectInstance);

      // projects are a bit different than standard addons, in that they have
      // possibly a CLI addon and internal addons. Add those now.
      pkgInfo.project = projectInstance;

      if (projectInstance.cli && projectInstance.cli.root) {
        pkgInfo.cliInfo = this._readPackage(projectInstance.cli.root);
      }

      // add any internal addons in the project. Since internal addons are
      // optional (and only used some of the time anyway), we don't want to
      // create a PackageInfo unless there is really a directory at the
      // suggested location. The created addon may internally have errors,
      // as with any other PackageInfo.
      projectInstance.supportedInternalAddonPaths().forEach(internalAddonPath => {
        if (getRealDirectoryPath(internalAddonPath)) {
          pkgInfo.addInternalAddon(this._readPackage(internalAddonPath));
        }
      });

      this._resolveDependencies();
    }

    return pkgInfo;
  }

  /**
   * To support the project.reloadPkg method, we need the ability to flush
   * the cache and reload from the updated package.json.
   * There are some issues with doing this:
   *   - Because of the possible relationship between projects and their addons
   *     due to symlinks, it's not trivial to flush only the data related to a
   *     given project.
   *   - If an 'ember-build-cli.js' dynamically adds new projects to the cache,
   *     we will not necessarily get called again to redo the loading of those
   *     projects.
   * The solution, implemented here:
   *   - Keep track of the Project objects whose packages are loaded into the cache.
   *   - If a project is reloaded, flush the cache, then do loadPackage again
   *     for all the known Projects.
   *
   * @public
   * @method reloadProjects
   * @return null
   */
  reloadProjects() {
    let projects = this.projects.slice();
    this._clear();
    projects.forEach(project => this.loadProject(project));
  }

  /**
   * Do the actual processing of the root directory of an addon, when the addon
   * object already exists (i.e. the addon is acting as the root object of a
   * tree, like project does). We need the object in order to find the internal addons.
   * _readPackage takes care of the general processing of the root directory
   * and common locations for addons, filling the cache with each.
   *
   * Once all the addon processing is done, go back through all cache entries
   * to create references between the packageInfo objects.
   *
   * @public
   * @method loadAddon
   * @param addonInstance the instance of the Addon object to load package data
   * about into the cache.
   * @return {PackageInfo} the PackageInfo object for the given Addon object.
   * Note that if the addon path is already in the cache, that will be returned.
   * No copy is made.
   */
  loadAddon(addonInstance) {
    let pkgInfo = this._readPackage(addonInstance.root, addonInstance.pkg);

    // NOTE: the returned pkgInfo may contain errors, or may contain
    // other packages that have errors. We will try to process
    // things anyway.
    if (!pkgInfo.processed) {
      pkgInfo.addon = addonInstance;
      this._resolveDependencies();
    }

    return pkgInfo;
  }

  /**
   * Resolve the node_module dependencies across all packages after they have
   * been loaded into the cache, because we don't know when a particular package
   * will enter the cache.
   *
   * Since loadProject can be called multiple times for different projects,
   * we don't want to reprocess any packages that happen to be common
   * between them. We'll handle this by marking any packageInfo once it
   * has been processed here, then ignore it in any later processing.
   *
   * @private
   * @method _resolveDependencies
   */
  _resolveDependencies() {
    let packageInfos = this._getPackageInfos();
    packageInfos.forEach(pkgInfo => {
      if (!pkgInfo.processed) {
        let pkgs = pkgInfo.addDependencies(pkgInfo.pkg.dependencies);
        if (pkgs) {
          pkgInfo.dependencyPackages = pkgs;
        }

        // for Projects only, we also add the devDependencies
        if (pkgInfo.project) {
          pkgs = pkgInfo.addDependencies(pkgInfo.pkg.devDependencies);
          if (pkgs) {
            pkgInfo.devDependencyPackages = pkgs;
          }
        }

        pkgInfo.processed = true;
      }
    });
  }

  /**
   * Add an entry to the cache.
   *
   * @private
   * @method _addEntry
   */
  _addEntry(path, entry) {
    this.entries[path] = entry;
  }

  /**
   * Retrieve an entry from the cache.
   *
   * @public
   * @method getEntry
   * @param {String} path the real path whose PackageInfo or NodeModulesList is desired.
   * @return {PackageInfo} or {NodeModulesList} the desired entry.
   */
  getEntry(path) {
    return this.entries[path];
  }

  /**
   * Indicate if an entry for a given path exists in the cache.
   *
   * @public
   * @method contains
   * @param {String} path the real path to check for in the cache.
   * @return true if the entry is present for the given path, false otherwise.
   */
  contains(path) {
    return this.entries[path] !== undefined;
  }

  _getPackageInfos() {
    let result = [];

    Object.keys(this.entries).forEach(path => {
      let entry = this.entries[path];
      if (entry instanceof PackageInfo) {
        result.push(entry);
      }
    });

    return result;
  }

  /*
   * Find a PackageInfo cache entry with the given path. If there is
   * no entry in the startPath, do as done in resolve.sync() - travel up
   * the directory hierarchy, attaching 'node_modules' to each directory and
   * seeing if the directory exists and has the relevant entry.
   *
   * We'll do things a little differently, though, for speed.
   *
   * If there is no cache entry, we'll try to use _readNodeModulesList to create
   * a new cache entry and its contents. If the directory does not exist,
   * We'll create a NodeModulesList cache entry anyway, just so we don't have
   * to check with the file system more than once for that directory (we
   * waste a bit of space, but gain speed by not hitting the file system
   * again for that path).
   * Once we have a NodeModulesList, check for the package name, and continue
   * up the path until we hit the root or the PackageInfo is found.
   *
   * @private
   * @method _findPackage
   * @param {String} packageName the name/path of the package to search for
   * @param {String} the path of the directory to start searching from
   */
  _findPackage(packageName, startPath) {
    let parsedPath = path.parse(startPath);
    let root = parsedPath.root;

    let currPath = startPath;

    while (currPath !== root) {
      let endsWithNodeModules = path.basename(currPath) === 'node_modules';

      let nodeModulesPath = endsWithNodeModules
        ? currPath
        : `${currPath}${path.sep}node_modules`;

      let nodeModulesList = this._readNodeModulesList(nodeModulesPath);

      // _readNodeModulesList only returns a NodeModulesList or null
      if (nodeModulesList) {
        let pkg = nodeModulesList.findPackage(packageName);
        if (pkg) {
          return pkg;
        }
      }

      currPath = path.dirname(currPath);
    }

    return null;
  }

  /**
   * Given a directory that supposedly contains a package, create a PackageInfo
   * object and try to fill it out, EVEN IF the package.json is not readable.
   * Errors will then be stored in the PackageInfo for anything with the package
   * that might be wrong.
   * Because it's possible that the path given to the packageDir is not actually valid,
   * we'll just use the path.resolve() version of that path to search for the
   * path in the cache, before trying to get the 'real' path (which also then
   * resolves links). The cache itself is keyed on either the realPath, if the
   * packageDir is actually a real valid directory path, or the normalized path (before
   * path.resolve()), if it is not.
   *
   * NOTE: the cache is also used to store the NULL_PROJECT project object,
   * which actually has no package.json or other files, but does have an empty
   * package object. Because of that, and to speed up processing, loadProject()
   * will pass in both the package root directory path and the project's package
   * object, if there is one. If the package object is present, we will use that
   * in preference to trying to find a package.json file.
   *
   * If there is no package object, and there is no package.json or the package.json
   * is bad or the package is an addon with
   * no main, the only thing we can do is return an ErrorEntry to the caller.
   * Once past all those problems, if any error occurs with any of the contents
   * of the package, they'll be cached in the PackageInfo itself.
   *
   * In summary, only PackageInfo or ErrorEntry will be returned.
   *
   * @private
   * @method _readPackage
   * @param {String} pkgDir the path of the directory to read the package.json from and
   *  process the contents and create a new cache entry or entries.
   * @param {Boolean} isRoot, for when this is to be considered the root
   * package, whose dependencies we must all consider for discovery.
   */
  _readPackage(packageDir, pkg, isRoot) {
    let normalizedPackageDir = path.normalize(packageDir);

    // Most of the time, normalizedPackageDir is already a real path (i.e. fs.realpathSync
    // will return the same value as normalizedPackageDir if the dir actually exists).
    // Because of that, we'll assume we can test for normalizedPackageDir first and return
    // if we find it.
    let pkgInfo = this.getEntry(normalizedPackageDir);
    if (pkgInfo) {
      return pkgInfo;
    }

    // collect errors we hit while trying to create the PackageInfo object.
    // We'll load these into the object once it's created.
    let setupErrors = new ErrorList();

    // We don't already have an entry (bad or otherwise) at normalizedPackageDir. See if
    // we can actually find a real path (including resolving links if needed).
    let pathFailed = false;

    let realPath = getRealDirectoryPath(normalizedPackageDir);

    if (realPath) {
      if (realPath !== normalizedPackageDir) {
        // getRealDirectoryPath actually changed something in the path (e.g.
        // by resolving a symlink), so see if we have this entry.
        pkgInfo = this.getEntry(realPath);
        if (pkgInfo) {
          return pkgInfo;
        }
      } else {
        // getRealDirectoryPath is same as normalizedPackageDir, and we know already we
        // don't have an entry there, so we need to create one.
      }
    } else {
      // no realPath, so either nothing is at the path or it's not a directory.
      // We need to use normalizedPackageDir as the real path.
      pathFailed = true;
      setupErrors.addError(Errors.ERROR_PACKAGE_DIR_MISSING, normalizedPackageDir);
      realPath = normalizedPackageDir;
    }

    // at this point we have realPath set, we don't already have a PackageInfo
    // for the path, and the path may or may not actually correspond to a
    // valid directory (pathFailed tells us which). If we don't have a pkg
    // object already, we need to be able to read one, unless we also don't
    // have a path.
    if (!pkg) {
      if (!pathFailed) {
        // we have a valid realPath
        let packageJsonPath = path.join(realPath, PACKAGE_JSON);
        let pkgfile = getRealFilePath(packageJsonPath);
        if (pkgfile) {
          try {
            pkg = fs.readJsonSync(pkgfile);
          } catch (e) {
            setupErrors.addError(Errors.ERROR_PACKAGE_JSON_PARSE, pkgfile);
          }
        } else {
          setupErrors.addError(Errors.ERROR_PACKAGE_JSON_MISSING, packageJsonPath);
        }
      }

      // Some error has occurred resulting in no pkg object, so just
      // create an empty one so we have something to use below.
      if (!pkg) {
        pkg = Object.create(null);
      }
    }

    // For storage, force the pkg.root to the calculated path. This will
    // save us from issues where we have a package for a non-existing
    // path and other stuff.
    pkg.root = realPath;

    // Create a new PackageInfo and load any errors as needed.
    // Note that pkg may be an empty object here.
    pkgInfo = new PackageInfo(pkg, realPath, this, isRoot);

    if (setupErrors.hasErrors()) {
      pkgInfo.errors = setupErrors;
      pkgInfo.valid = false;
    }

    // If we have an ember-addon, check that the main exists and points
    // to a valid file.
    if (pkgInfo.isAddon()) {

      // Note: when we have both 'main' and ember-addon:main, the latter takes precedence
      let main = (pkg['ember-addon'] && pkg['ember-addon'].main) || pkg['main'];

      if (!main || main === '.' || main === './') {
        main = 'index.js';
      } else if (!path.extname(main)) {
        main = `${main}.js`;
      }

      pkg.main = main;

      let mainPath = path.join(realPath, main);
      let mainRealPath = getRealFilePath(mainPath);

      if (mainRealPath) {
        pkgInfo.addonMainPath = mainRealPath;
      } else {
        pkgInfo.addError(Errors.ERROR_EMBER_ADDON_MAIN_MISSING, mainPath);
        this.valid = false;
      }
    }

    // The packageInfo itself is now "complete", though we have not
    // yet dealt with any of its "child" packages. Add it to the
    // cache
    this._addEntry(realPath, pkgInfo);

    let emberAddonInfo = pkg['ember-addon'];

    // Set up packageInfos for any in-repo addons
    if (emberAddonInfo) {
      let paths = emberAddonInfo.paths;

      if (paths) {
        paths.forEach(p => {
          let addonPath = path.join(realPath, p); // real path, though may not exist.
          let addonPkgInfo = this._readPackage(addonPath); // may have errors in the addon package.
          pkgInfo.addInRepoAddon(addonPkgInfo);
        });
      }
    }

    if (pkgInfo.mayHaveAddons) {
      // read addon modules from node_modules. We read the whole directory
      // because it's assumed that npm/yarn may have placed addons in the
      // directory from lower down in the project tree, and we want to get
      // the data into the cache ASAP. It may not necessarily be a 'real' error
      // if we find an issue, if nobody below is actually invoking the addon.
      let nodeModules = this._readNodeModulesList(path.join(realPath, 'node_modules'));

      if (nodeModules) {
        pkgInfo.nodeModules = nodeModules;
      }
    } else {
      // will not have addons, so even if there are node_modules here, we can
      // simply pretend there are none.
      pkgInfo.nodeModules = NodeModulesList.NULL;
    }

    return pkgInfo;
  }

  /**
   * Process a directory of modules in a given package directory.
   *
   * We will allow cache entries for node_modules that actually
   * have no contents, just so we don't have to hit the file system more
   * often than necessary--it's much quicker to check an in-memory object.
   * object.
   *
   * Note: only a NodeModulesList or null is returned.
   *
   * @private
   * @method _readModulesList
   * @param {String} nodeModulesDir the path of the node_modules directory
   *  to read the package.json from and process the contents and create a
   *  new cache entry or entries.
   */
  _readNodeModulesList(nodeModulesDir) {
    let normalizedNodeModulesDir = path.normalize(nodeModulesDir);

    // Much of the time, normalizedNodeModulesDir is already a real path (i.e.
    // fs.realpathSync will return the same value as normalizedNodeModulesDir, if
    // the directory actually exists). Because of that, we'll assume
    // we can test for normalizedNodeModulesDir first and return if we find it.
    let nodeModulesList = this.getEntry(normalizedNodeModulesDir);
    if (nodeModulesList) {
      return nodeModulesList;
    }

    // NOTE: because we call this when searching for objects in node_modules
    // directories that may not exist, we'll just return null here if the
    // directory is not real. If it actually is an error in some case,
    // the caller can create the error there.
    let realPath = getRealDirectoryPath(normalizedNodeModulesDir);

    if (!realPath) {
      return null;
    }

    // realPath may be different than the original normalizedNodeModulesDir, so
    // we need to check the cache again.
    if (realPath !== normalizedNodeModulesDir) {
      nodeModulesList = this.getEntry(realPath);
      if (nodeModulesList) {
        return nodeModulesList;
      }
    } else {
      // getRealDirectoryPath is same as normalizedPackageDir, and we know already we
      // don't have an entry there, so we need to create one.
    }

    // At this point we know the directory node_modules exists and we can
    // process it. Further errors will be recorded here, or in the objects
    // that correspond to the node_modules entries.
    nodeModulesList = new NodeModulesList(realPath, this);

    let entries = fs.readdirSync(realPath); // should not fail because getRealDirectoryPath passed

    entries.forEach(entryName => {
      // entries should be either a package or a scoping directory. I think
      // there can also be files, but we'll ignore those.

      if (entryName.startsWith('.') || entryName.startsWith('_')) {
        // we explicitly want to ignore these, according to the
        // definition of a valid package name.
        return;
      }

      let entryPath = path.join(realPath, entryName);

      if (getRealFilePath(entryPath)) {
        // we explicitly want to ignore valid regular files in node_modules.
        // This is a bit slower than just checking for directories, but we need to be sure.
        return;
      }

      // At this point we have an entry name that should correspond to
      // a directory, which should turn into either a NodeModulesList or
      // PackageInfo. If not, it's an error on this NodeModulesList.
      let entryVal;

      if (entryName.startsWith('@')) {
        // we should have a scoping directory.
        entryVal = this._readNodeModulesList(entryPath);

        // readModulesDir only returns NodeModulesList or null
        if (entryVal instanceof NodeModulesList) {
          nodeModulesList.addEntry(entryName, entryVal);
        } else {
          // This (null return) really should not occur, unless somehow the
          // dir disappears between the time of fs.readdirSync and now.
          nodeModulesList.addError(Errors.ERROR_NODEMODULES_ENTRY_MISSING, entryName);
        }
      } else {
        // we should have a package. We will always get a PackageInfo
        // back, though it may contain errors.
        entryVal = this._readPackage(entryPath);
        nodeModulesList.addEntry(entryName, entryVal);
      }
    });

    this._addEntry(realPath, nodeModulesList);

    return nodeModulesList;
  }
}

module.exports = PackageInfoCache;

// export the getRealXXXPath functions to enable testing
module.exports.getRealFilePath = getRealFilePath;
module.exports.getRealDirectoryPath = getRealDirectoryPath;
