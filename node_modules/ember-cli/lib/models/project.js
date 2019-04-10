'use strict';

/**
@module ember-cli
*/
const RSVP = require('rsvp');
const path = require('path');
const findup = require('find-up');
const resolve = RSVP.denodeify(require('resolve'));
const fs = require('fs-extra');
const _ = require('ember-cli-lodash-subset');
const logger = require('heimdalljs-logger')('ember-cli:project');
const versionUtils = require('../utilities/version-utils');
const emberCLIVersion = versionUtils.emberCLIVersion;
const findAddonByName = require('../utilities/find-addon-by-name');
const heimdall = require('heimdalljs');
const PackageInfoCache = require('../models/package-info-cache');

const instantiateAddons = require('../models/instantiate-addons');

let processCwd = process.cwd();

// ensure NULL_PROJECT is a singleton
let NULL_PROJECT;

class Project {
  /**
    The Project model is tied to your package.json. It is instantiated
    by giving {{#crossLink "Project/closestSync:method"}}{{/crossLink}}
    the path to your project.

    @class Project
    @constructor
    @param {String} root Root directory for the project
    @param {Object} pkg  Contents of package.json
    @param {UI} ui
    @param {CLI} cli
  */
  constructor(root, pkg, ui, cli) {
    logger.info('init root: %s', root);

    this.root = root;
    this.pkg = pkg;
    this.ui = ui;
    this.cli = cli;
    this.addonPackages = Object.create(null);
    this.addons = [];
    this.liveReloadFilterPatterns = [];
    this.setupBowerDirectory();
    this.configCache = new Map();

    /**
      Set when the `Watcher.detectWatchman` helper method finishes running,
      so that other areas of the system can be aware that watchman is being used.

      For example, this information is used in the broccoli build pipeline to know
      if we can watch additional directories (like bower_components) "cheaply".

      Contains `enabled` and `version`.

      @private
      @property _watchmanInfo
      @return {Object}
      @default false
    */
    this._watchmanInfo = {
      enabled: false,
      version: null,
      canNestRoots: false,
    };

    let instrumentation = this._instrumentation = ensureInstrumentation(cli, ui);
    instrumentation.project = this;

    this.emberCLIVersion = emberCLIVersion;

    this._nodeModulesPath = null;

    if (this.cli && this.cli.packageInfoCache) {
      this.packageInfoCache = this.cli.packageInfoCache;
    } else {
      this.packageInfoCache = new PackageInfoCache(this.ui);
    }

    // we're not dealing with NULL_PROJECT (note that it has not
    // be set yet, so we can't compare to that var.)
    this._packageInfo = this.packageInfoCache.loadProject(this);

    // force us to use the real path as the root.
    this.root = this._packageInfo.realPath;

    // XXX Need to decide what to do here about showing errors. For
    // a non-CLI project the cache is local and probably should. For
    // a CLI project the cache is there, but not sure when we'll know
    // about all the errors, because there may be multiple projects.
    if (this.packageInfoCache.hasErrors()) {
      this.packageInfoCache.showErrors();
    }
  }

  /**
   * Sets the name of the bower directory for this project
   *
   * @private
   * @method setupBowerDirectory
   */
  setupBowerDirectory() {
    let bowerrcPath = path.join(this.root, '.bowerrc');

    logger.info('bowerrc path: %s', bowerrcPath);

    if (fs.existsSync(bowerrcPath)) {
      try {
        this.bowerDirectory = fs.readJsonSync(bowerrcPath).directory;
      } catch (exception) {
        logger.info('failed to parse bowerc: %s', exception);
        this.bowerDirectory = null;
      }
    }

    this.bowerDirectory = this.bowerDirectory || 'bower_components';
    logger.info('bowerDirectory: %s', this.bowerDirectory);
  }

  // Checks whether the project's npm dependencies are
  // present. Previously this just looked for a node_modules folder in
  // a fixed place (which is not compatible with node's module
  // resolution algorithm). Now we just sample to see if we can
  // resolve the first dependency or devDependency we find.
  hasDependencies() {
    if (this.cli.disableDependencyChecker) {
      // Blueprint tests set this flag.
      return true;
    }

    for (let depName in this.dependencies()) {
      try {
        this.resolveSync(`${depName}/package.json`);
        return true;
      } catch (err) {
        if (err.code !== 'MODULE_NOT_FOUND') {
          throw err;
        }
        return false;
      }
    }
    return false;
  }

  static nullProject(ui, cli) {
    if (NULL_PROJECT) { return NULL_PROJECT; }

    NULL_PROJECT = new Project(processCwd, {}, ui, cli);

    NULL_PROJECT.isEmberCLIProject = function() {
      return false;
    };

    NULL_PROJECT.isEmberCLIAddon = function() {
      return false;
    };

    NULL_PROJECT.isModuleUnification = function() {
      return false;
    };

    NULL_PROJECT.name = function() {
      return path.basename(process.cwd());
    };

    NULL_PROJECT.initializeAddons();

    return NULL_PROJECT;
  }

  /**
    Returns the name from package.json.

    @private
    @method name
    @return {String} Package name
   */
  name() {
    const getPackageBaseName = require('../utilities/get-package-base-name');

    return getPackageBaseName(this.pkg.name);
  }

  /**
    Returns whether or not this is an Ember CLI project.
    This checks whether ember-cli is listed in devDependencies.

    @private
    @method isEmberCLIProject
    @return {Boolean} Whether this is an Ember CLI project
   */
  isEmberCLIProject() {
    return (this.cli ? this.cli.npmPackage : 'ember-cli') in this.dependencies();
  }

  /**
    Returns whether or not this is an Ember CLI addon.

    @method isEmberCLIAddon
    @return {Boolean} Whether or not this is an Ember CLI Addon.
   */
  isEmberCLIAddon() {
    return !!this.pkg.keywords && this.pkg.keywords.indexOf('ember-addon') > -1;
  }

  /**
     Returns whether this is using a module unification format.
     @private
     @method isModuleUnification
     @return {Boolean} Whether this is using a module unification format.
    */
  isModuleUnification() {
    const { isExperimentEnabled } = require('../experiments');
    if (isExperimentEnabled('MODULE_UNIFICATION')) {
      return this.has('src');
    } else {
      return false;
    }
  }

  /**
    Returns the path to the configuration.

    @private
    @method configPath
    @return {String} Configuration path
   */
  configPath() {
    let configPath = 'config';

    if (this.pkg['ember-addon'] && this.pkg['ember-addon']['configPath']) {
      configPath = this.pkg['ember-addon']['configPath'];
    }

    return path.join(this.root, configPath, 'environment');
  }

  /**
    Loads the configuration for this project and its addons.

    @public
    @method config
    @param  {String} env Environment name
    @return {Object}     Merged configuration object
   */
  config(env) {
    let key = `${this.configPath()}|${env}`;
    let config = this.configCache.get(key);
    if (config === undefined) {
      config = this.configWithoutCache(env);
      this.configCache.set(key, config);
    }
    return _.cloneDeep(config);
  }

  /**
   * @private
   * @method configWithoutCache
   * @param  {String} env Environment name
   * @return {Object}     Merged confiration object
   */
  configWithoutCache(env) {
    let configPath = this.configPath();

    if (fs.existsSync(`${configPath}.js`)) {
      let appConfig = this.require(configPath)(env);
      let addonsConfig = this.getAddonsConfig(env, appConfig);

      return _.merge(addonsConfig, appConfig);
    } else {
      return this.getAddonsConfig(env, {});
    }
  }

  /**
    Returns the targets of this project, or the default targets if not present.

    @public
    @property targets
    @return {Object}  Targets object
  */
  get targets() {
    if (this._targets) {
      return this._targets;
    }
    let configPath = 'config';

    if (this.pkg['ember-addon'] && this.pkg['ember-addon']['configPath']) {
      configPath = this.pkg['ember-addon']['configPath'];
    }

    let targetsPath = path.join(this.root, configPath, 'targets');

    if (fs.existsSync(`${targetsPath}.js`)) {
      this._targets = this.require(targetsPath);
    } else {
      this._targets = require('../utilities/default-targets');
    }
    return this._targets;
  }

  /**
    Returns the addons configuration.

    @private
    @method getAddonsConfig
    @param  {String} env       Environment name
    @param  {Object} appConfig Application configuration
    @return {Object}           Merged configuration of all addons
   */
  getAddonsConfig(env, appConfig) {
    this.initializeAddons();

    let initialConfig = _.merge({}, appConfig);

    return this.addons.reduce((config, addon) => {
      if (addon.config) {
        _.merge(config, addon.config(env, config));
      }

      return config;
    }, initialConfig);
  }

  /**
    Returns whether or not the given file name is present in this project.

    @private
    @method has
    @param  {String}  file File name
    @return {Boolean}      Whether or not the file is present
   */
  has(file) {
    return fs.existsSync(path.join(this.root, file)) || fs.existsSync(path.join(this.root, `${file}.js`));
  }

  /**
    Resolves the absolute path to a file synchronously

    @private
    @method resolveSync
    @param  {String} file File to resolve
    @return {String}      Absolute path to file
   */
  resolveSync(file) {
    return resolve.sync(file, {
      basedir: process.env.EMBER_NODE_PATH || this.root,
    });
  }

  /**
    Calls `require` on a given module from the context of the project. For
    instance, an addon may want to require a class from the root project's
    version of ember-cli.

    @public
    @method require
    @param  {String} file File path or module name
    @return {Object}      Imported module
   */
  require(file) {
    let path = this.resolveSync(file);
    return require(path);
  }

  /**
    Returns the dependencies from a package.json

    @private
    @method dependencies
    @param  {Object} [pkg=this.pkg] Package object
    @param  {Boolean} [excludeDevDeps=false] Whether or not development dependencies should be excluded
    @return {Object} Dependencies
   */
  dependencies(pkg, excludeDevDeps) {
    pkg = pkg || this.pkg || {};

    let devDependencies = pkg['devDependencies'];
    if (excludeDevDeps) {
      devDependencies = {};
    }

    return Object.assign({}, devDependencies, pkg['dependencies']);
  }

  /**
    Returns the bower dependencies for this project.

    @private
    @method bowerDependencies
    @param  {String} bower Path to bower.json
    @return {Object}       Bower dependencies
   */
  bowerDependencies(bower) {
    if (!bower) {
      let bowerPath = path.join(this.root, 'bower.json');
      bower = (fs.existsSync(bowerPath)) ? require(bowerPath) : {};
    }
    return Object.assign({}, bower['devDependencies'], bower['dependencies']);
  }

  /**
    Provides the list of paths to consult for addons that may be provided
    internally to this project. Used for middleware addons with built-in support.

    @private
    @method supportedInternalAddonPaths
  */
  supportedInternalAddonPaths() {
    if (!this.root) { return []; }

    let internalMiddlewarePath = path.join(__dirname, '../tasks/server/middleware');
    let internalTransformPath = path.join(__dirname, '../tasks/transforms');

    return [
      path.join(internalMiddlewarePath, 'testem-url-rewriter'),
      path.join(internalMiddlewarePath, 'tests-server'),
      path.join(internalMiddlewarePath, 'history-support'),
      path.join(internalMiddlewarePath, 'broccoli-watcher'),
      path.join(internalMiddlewarePath, 'broccoli-serve-files'),
      path.join(internalMiddlewarePath, 'proxy-server'),
      path.join(internalTransformPath, 'amd'),
    ];
  }

  /**
   * Discovers all addons for this project and stores their names and
   * package.json contents in this.addonPackages as key-value pairs.
   *
   * Any packageInfos that we find that are marked as not valid are excluded.
   *
   * @private
   * @method discoverAddons
   */
  discoverAddons() {
    if (this._didDiscoverAddons) {
      this._didDiscoverAddons = true;
      return;
    }

    let pkgInfo = this.packageInfoCache.getEntry(this.root);

    let addonPackageList = pkgInfo.discoverProjectAddons();
    this.addonPackages = pkgInfo.generateAddonPackages(addonPackageList);

    // in case any child addons are invalid, dump to the console about them.
    pkgInfo.dumpInvalidAddonPackages(addonPackageList);
  }

  /**
    Loads and initializes all addons for this project.

    @private
    @method initializeAddons
   */
  initializeAddons() {
    if (this._addonsInitialized) {
      return;
    }
    this._addonsInitialized = true;

    logger.info('initializeAddons for: %s', this.name());

    this.discoverAddons();

    this.addons = instantiateAddons(this, this, this.addonPackages);
    this.addons.forEach(addon => logger.info('addon: %s', addon.name));
  }

  /**
    Returns what commands are made available by addons by inspecting
    `includedCommands` for every addon.

    @private
    @method addonCommands
    @return {Object} Addon names and command maps as key-value pairs
   */
  addonCommands() {
    const Command = require('../models/command');
    let commands = Object.create(null);
    this.addons.forEach(addon => {
      if (!addon.includedCommands) { return; }

      let token = heimdall.start({
        name: `lookup-commands: ${addon.name}`,
        addonName: addon.name,
        addonCommandInitialization: true,
      });

      let includedCommands = addon.includedCommands();
      let addonCommands = Object.create(null);

      for (let key in includedCommands) {
        if (typeof includedCommands[key] === 'function') {
          addonCommands[key] = includedCommands[key];
        } else {
          addonCommands[key] = Command.extend(includedCommands[key]);
        }
      }
      if (Object.keys(addonCommands).length) {
        commands[addon.name] = addonCommands;
      }

      token.stop();
    });
    return commands;
  }

  /**
    Execute a given callback for every addon command.
    Example:

    ```
    project.eachAddonCommand(function(addonName, commands) {
      console.log('Addon ' + addonName + ' exported the following commands:' + commands.keys().join(', '));
    });
    ```

    @private
    @method eachAddonCommand
    @param  {Function} callback [description]
   */
  eachAddonCommand(callback) {
    if (this.initializeAddons && this.addonCommands) {
      this.initializeAddons();
      let addonCommands = this.addonCommands();

      _.forOwn(addonCommands, (commands, addonName) => callback(addonName, commands));
    }
  }

  /**
    Path to the blueprints for this project.

    @private
    @method localBlueprintLookupPath
    @return {String} Path to blueprints
   */
  localBlueprintLookupPath() {
    return path.join(this.root, 'blueprints');
  }

  /**
    Returns a list of paths (including addon paths) where blueprints will be looked up.

    @private
    @method blueprintLookupPaths
    @return {Array} List of paths
   */
  blueprintLookupPaths() {
    if (this.isEmberCLIProject()) {
      let lookupPaths = [this.localBlueprintLookupPath()];
      let addonLookupPaths = this.addonBlueprintLookupPaths();

      return lookupPaths.concat(addonLookupPaths);
    } else {
      return this.addonBlueprintLookupPaths();
    }
  }

  /**
    Returns a list of addon paths where blueprints will be looked up.

    @private
    @method addonBlueprintLookupPaths
    @return {Array} List of paths
   */
  addonBlueprintLookupPaths() {
    let addonPaths = this.addons.reduce((sum, addon) => {
      if (addon.blueprintsPath) {
        let val = addon.blueprintsPath();
        if (val) { sum.push(val); }
      }
      return sum;
    }, []).reverse();

    return addonPaths;
  }

  /**
    Reloads package.json

    @private
    @method reloadPkg
    @return {Object} Package content
   */
  reloadPkg() {
    let pkgPath = path.join(this.root, 'package.json');

    // We use readFileSync instead of require to avoid the require cache.
    this.pkg = fs.readJsonSync(pkgPath);

    this.packageInfoCache.reloadProjects();

    return this.pkg;
  }

  /**
    Re-initializes addons.

    @private
    @method reloadAddons
   */
  reloadAddons() {
    this.reloadPkg();
    this._addonsInitialized = false;
    return this.initializeAddons();
  }

  /**
    Find an addon by its name

    @private
    @method findAddonByName
    @param  {String} name Addon name as specified in package.json
    @return {Addon}       Addon instance
   */
  findAddonByName(name) {
    this.initializeAddons();

    return findAddonByName(this.addons, name);
  }

  /**
    Generate test file contents.

    This method is supposed to be overwritten by test framework addons
    like `ember-qunit` and `ember-mocha`.

    @public
    @method generateTestFile
    @param {String} moduleName Name of the test module (e.g. `JSHint`)
    @param {Object[]} tests Array of tests with `name`, `passed` and `errorMessage` properties
    @return {String} The test file content
   */
  generateTestFile() {
    let message = 'Please install an Ember.js test framework addon or update your dependencies.';

    if (this.ui) {
      this.ui.writeDeprecateLine(message);
    } else {
      console.warn(message);
    }

    return '';
  }

  /**
    Returns a new project based on the first package.json that is found
    in `pathName`.

    @private
    @static
    @method closestSync
    @param  {String} pathName Path to your project
    @param  {UI} _ui The UI instance to provide to the created Project.
    @return {Project}         Project instance
   */
  static closestSync(pathName, _ui, _cli) {
    logger.info('looking for package.json starting at %s', pathName);

    let ui = ensureUI(_ui);

    let directory = findupPath(pathName);
    logger.info('found package.json at %s', directory);

    let relative = path.relative(directory, pathName);
    if (relative.indexOf('tmp') === 0) {
      logger.info('ignoring parent project since we are in the tmp folder of the project');
      return Project.nullProject(_ui, _cli);
    }

    let pkg = fs.readJsonSync(path.join(directory, 'package.json'));
    logger.info('project name: %s', pkg && pkg.name);

    if (!isEmberCliProject(pkg)) {
      logger.info('ignoring parent project since it is not an ember-cli project');
      return Project.nullProject(_ui, _cli);
    }

    return new Project(directory, pkg, ui, _cli);
  }

  /**
    Returns a new project based on the first package.json that is found
    in `pathName`, or the nullProject.

    The nullProject signifies no-project, but abides by the null object pattern

    @private
    @static
    @method projectOrnullProject
    @param  {UI} _ui The UI instance to provide to the created Project.
    @return {Project}         Project instance
   */
  static projectOrnullProject(_ui, _cli) {
    try {
      return Project.closestSync(process.cwd(), _ui, _cli);
    } catch (reason) {
      if (reason instanceof Project.NotFoundError) {
        return Project.nullProject(_ui, _cli);
      } else {
        throw reason;
      }
    }
  }

  /**
    Returns the project root based on the first package.json that is found

    @static
    @method getProjectRoot
    @return {String} The project root directory
   */
  static getProjectRoot() {
    let packagePath = findup.sync('package.json');
    if (!packagePath) {
      logger.info('getProjectRoot: not found. Will use cwd: %s', process.cwd());
      return process.cwd();
    }

    let directory = path.dirname(packagePath);
    const pkg = require(packagePath);

    if (pkg && pkg.name === 'ember-cli') {
      logger.info('getProjectRoot: named \'ember-cli\'. Will use cwd: %s', process.cwd());
      return process.cwd();
    }

    logger.info('getProjectRoot %s -> %s', process.cwd(), directory);
    return directory;
  }
}

class NotFoundError extends Error {
  constructor(message) {
    super(message);
    this.name = 'NotFoundError';
    this.stack = (new Error()).stack;
  }
}

Project.NotFoundError = NotFoundError;

function ensureInstrumentation(cli, ui) {
  if (cli && cli.instrumentation) {
    return cli.instrumentation;
  }

  // Instrumentation `require` needs to occur inline due to circular dependencies between
  // Instrumentation => getConfig => Project => Instrumentation. getConfig is used in Project to
  // get the project root.
  let Instrumentation = require('./instrumentation');
  // created without a `cli` object (possibly from deprecated `Brocfile.js`)
  return new Instrumentation({ ui, initInstrumentation: null });
}

function ensureUI(_ui) {
  let ui = _ui;

  if (!ui) {
    // TODO: one UI (lib/cli/index.js also has one for now...)
    const UI = require('console-ui');
    ui = new UI({
      inputStream: process.stdin,
      outputStream: process.stdout,
      ci: process.env.CI || (/^(dumb|emacs)$/).test(process.env.TERM),
      writeLevel: (process.argv.indexOf('--silent') !== -1) ? 'ERROR' : undefined,
    });
  }

  return ui;
}

function findupPath(pathName) {
  let pkgPath = findup.sync('package.json', { cwd: pathName });
  if (!pkgPath) {
    throw new NotFoundError(`No project found at or up from: \`${pathName}\``);
  }

  return path.dirname(pkgPath);
}

function isEmberCliProject(pkg) {
  return pkg && (
    (pkg.dependencies && Object.keys(pkg.dependencies).indexOf('ember-cli') !== -1) ||
    (pkg.devDependencies && Object.keys(pkg.devDependencies).indexOf('ember-cli') !== -1)
  );
}

// Export
module.exports = Project;
