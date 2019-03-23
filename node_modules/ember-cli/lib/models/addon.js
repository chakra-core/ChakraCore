'use strict';

/**
@module ember-cli
*/

const fs = require('fs');
const path = require('path');
const SilentError = require('silent-error');
const heimdallLogger = require('heimdalljs-logger');
const logger = heimdallLogger('ember-cli:addon');
const treeCacheLogger = heimdallLogger('ember-cli:addon:tree-cache');
const cacheKeyLogger = heimdallLogger('ember-cli:addon:cache-key-for-tree');
const { isExperimentEnabled } = require('../experiments');
const PackageInfoCache = require('../models/package-info-cache');

const p = require('ember-cli-preprocess-registry/preprocessors');
const preprocessJs = p.preprocessJs;
const preprocessCss = p.preprocessCss;
const preprocessTemplates = p.preprocessTemplates;

const instantiateAddons = require('../models/instantiate-addons');

const CoreObject = require('core-object');
const Project = require('../models/project');

const mergeTrees = require('../broccoli/merge-trees');
const Funnel = require('broccoli-funnel');
const walkSync = require('walk-sync');
const ensurePosixPath = require('ensure-posix-path');
const defaultsDeep = require('ember-cli-lodash-subset').defaultsDeep;
const findAddonByName = require('../utilities/find-addon-by-name');
const heimdall = require('heimdalljs');
const calculateCacheKeyForTree = require('calculate-cache-key-for-tree');
const addonProcessTree = require('../utilities/addon-process-tree');
const emberCLIBabelConfigKey = require('../utilities/ember-cli-babel-config-key');
const semver = require('semver');
const processModulesOnly = require('../broccoli/babel-process-modules-only');
const registryHasPreprocessor = require('../utilities/registry-has-preprocessor');

const BUILD_BABEL_OPTIONS_FOR_PREPROCESSORS = Symbol('BUILD_BABEL_OPTIONS_FOR_PREPROCESSORS');

if (!heimdall.hasMonitor('addon-tree-cache')) {
  heimdall.registerMonitor('addon-tree-cache', function AddonTreeCacheSchema() {
    this.hits = 0;
    this.misses = 0;
    this.adds = 0;
  });
}

if (!heimdall.hasMonitor('cache-key-for-tree')) {
  heimdall.registerMonitor('cache-key-for-tree', function CacheKeyForTreeSchema() {
    this.modifiedMethods = 0;
    this.treeForMethodsOverride = 0;
  });
}

let DEFAULT_TREE_FOR_METHODS = {
  app: 'treeForApp',
  src: 'treeForSrc',
  addon: 'treeForAddon',
  'addon-styles': 'treeForAddonStyles',
  'addon-templates': 'treeForAddonTemplates',
  'addon-test-support': 'treeForAddonTestSupport',
  public: 'treeForPublic',
  styles: 'treeForStyles',
  templates: 'treeForTemplates',
  'test-support': 'treeForTestSupport',
  vendor: 'treeForVendor',
};

let GLOBAL_TREE_FOR_METHOD_METHODS = ['treeFor', '_treeFor', 'treeGenerator'];
let DEFAULT_TREE_FOR_METHOD_METHODS = {
  app: ['treeForApp'],
  src: ['treeForSrc'],
  addon: [
    'treeForAddon', 'treeForAddonStyles', 'treeForAddonTemplates',
    'compileAddon', 'processedAddonJsFiles', 'compileTemplates',
    '_addonTemplateFiles', 'compileStyles', 'preprocessJs',
    'addonJsFiles',
  ],
  'addon-styles': ['treeForAddonStyles'],
  'addon-templates': ['treeForAddonTemplates'],
  'addon-test-support': ['treeForAddonTestSupport', 'preprocessJs'],
  public: ['treeForPublic'],
  styles: ['treeForStyles'],
  templates: ['treeForTemplates'],
  'test-support': ['treeForTestSupport'],
  vendor: ['treeForVendor'],
};

let ADDON_TREE_CACHE = {
  __cache: Object.create(null),

  getItem(key) {
    let addonTreeCacheStats = heimdall.statsFor('addon-tree-cache');
    let cachedValue = this.__cache[key];

    if (cachedValue) {
      addonTreeCacheStats.hits++;
      treeCacheLogger.info(`Cache Hit: ${key}`);
      return cachedValue;
    } else {
      addonTreeCacheStats.misses++;
      treeCacheLogger.info(`Cache Miss: ${key}`);
      return null;
    }
  },

  setItem(key, value) {
    let hasValue = !!value;
    heimdall.statsFor('addon-tree-cache').adds++;
    treeCacheLogger.info(`Cache Add: ${key} - ${hasValue}`);
    this.__cache[key] = value;
  },

  clear() {
    this.__cache = Object.create(null);
  },
};

function _resetTreeCache() {
  ADDON_TREE_CACHE.clear();
}

function warn(message) {
  if (this.ui) {
    this.ui.writeDeprecateLine(message);
  } else {
    const chalk = require('chalk');
    console.log(chalk.yellow(`DEPRECATION: ${message}`));
  }
}

/**
  Root class for an Addon. If your addon module exports an Object this
  will be extended from this base class. If you export a constructor (function),
  it will **not** extend from this class.

  Hooks:

  - {{#crossLink "Addon/config:method"}}{{/crossLink}}
  - {{#crossLink "Addon/blueprintsPath:method"}}{{/crossLink}}
  - {{#crossLink "Addon/includedCommands:method"}}{{/crossLink}}
  - {{#crossLink "Addon/importTransforms:method"}}{{/crossLink}}
  - {{#crossLink "Addon/serverMiddleware:method"}}{{/crossLink}}
  - {{#crossLink "Addon/testemMiddleware:method"}}{{/crossLink}}
  - {{#crossLink "Addon/postBuild:method"}}{{/crossLink}}
  - {{#crossLink "Addon/preBuild:method"}}{{/crossLink}}
  - {{#crossLink "Addon/outputReady:method"}}{{/crossLink}}
  - {{#crossLink "Addon/buildError:method"}}{{/crossLink}}
  - {{#crossLink "Addon/included:method"}}{{/crossLink}}
  - {{#crossLink "Addon/shouldIncludeChildAddon:method"}}{{/crossLink}}
  - {{#crossLink "Addon/setupPreprocessorRegistry:method"}}{{/crossLink}}
  - {{#crossLink "Addon/preprocessTree:method"}}{{/crossLink}}
  - {{#crossLink "Addon/postprocessTree:method"}}{{/crossLink}}
  - {{#crossLink "Addon/lintTree:method"}}{{/crossLink}}
  - {{#crossLink "Addon/contentFor:method"}}{{/crossLink}}
  - {{#crossLink "Addon/treeFor:method"}}{{/crossLink}}

  @class Addon
  @extends CoreObject
  @constructor
  @param {Project|Addon} parent The project or addon that directly depends on this addon
  @param {Project} project The current project (deprecated)
*/
let addonProto = {
  /**
    The name of this addon.

    @public
    @final
    @type String
    @property name
  */

  /**
    The absolute path of the root directory where this addon is located.

    @public
    @final
    @type String
    @property root
  */

  /**
    The host app instance.

    **Note**: this property will only be present on addons that are a direct dependency
    of the application itself, not of other addons. It is also not available in `init()`,
    but will be set before `setupPreprocessorRegistry()` and `included()` are invoked.

    @public
    @final
    @type EmberApp
    @property app
  */

  /**
    The root {{#crossLink "Project"}}project{{/crossLink}} to which this addon belongs.

    @public
    @final
    @type Project
    @property project
  */

  /**
    This addon's parent.

    If the addon is a direct dependency of an application, then `parent` will be the
    corresponding {{#crossLink "Project"}}project{{/crossLink}} instance. If it's a
    dependency of another addon, then `parent` will be a reference to that addon.

    @public
    @final
    @type Project|Addon
    @property parent
  */

  /**
    The set of addons that this addon itself depends on.

    This array is populated from the addon's listed `dependencies` and any items in
    `ember-addon.paths` in its `package.json`.

    @public
    @final
    @type Addon[]
    @property addons
  */

  /**
    A [`console-ui`](https://github.com/ember-cli/console-ui) object that can be used
    to log messages for the user and indicate progress on long-running operations.

    @public
    @final
    @type UI
    @property ui
  */

  /**
    The contents of the addon's `package.json`.

    @public
    @final
    @type Object
    @property pkg
  */

  /**
    Initializes the addon.  If you override this method make sure and call `this._super.init && this._super.init.apply(this, arguments);` or your addon will not work.

    @public
    @method init
    @param {Project|Addon} parent The project or addon that directly depends on this addon
    @param {Project} project The current project (deprecated)

    @example
    ```js
    init(parent, project) {
      this._super.init && this._super.init.apply(this, arguments);
      this._someCustomSetup();
    }
    ```
  */
  init(parent, project) {
    this._super();
    this.parent = parent;
    this.project = project;
    this.ui = project && project.ui;
    this.addonPackages = Object.create(null);
    this.addons = [];
    this.registry = p.defaultRegistry(this);

    if (!this.root) {
      throw new Error('Addon classes must be instantiated with the `root` property');
    }

    if (!this.name) {
      throw new SilentError(`An addon must define a \`name\` property (found at ${this.root}).`);
    }

    this._nodeModulesPath = null;

    this.treePaths = {
      app: 'app',
      src: 'src',
      styles: 'app/styles',
      templates: 'app/templates',
      addon: 'addon',
      'addon-styles': 'addon/styles',
      'addon-templates': 'addon/templates',
      vendor: 'vendor',
      'test-support': 'test-support',
      'addon-test-support': 'addon-test-support',
      public: 'public',
    };

    this.treeForMethods = defaultsDeep({}, DEFAULT_TREE_FOR_METHODS);

    if (this.parent) {
      // The parent should already have a packageInfoCache.
      // Because it does, this package should already be in the cache.
      this.packageInfoCache = this.parent.packageInfoCache;
    }

    if (!this.packageInfoCache) {
      // this is most likely a 'root' addon, so create a new PackageInfoCache
      // for it so we get the PackageInfo for it and its children.
      //
      // this may also be someone who failed to mock their addon correctly in testing.
      this.packageInfoCache = new PackageInfoCache(this.ui);
    }

    this._packageInfo = this.packageInfoCache.loadAddon(this);

    // force us to use the real path as the root.
    this.root = this._packageInfo.realPath;

    p.setupRegistry(this);


    this._initDefaultBabelOptions();

    if (isExperimentEnabled('DELAYED_TRANSPILATION')) {
      this.registry.remove('template', 'ember-cli-htmlbars');
    }
  },

  _initDefaultBabelOptions() {
    this.__originalOptions = this.options = defaultsDeep(this.options, {
      babel: this[BUILD_BABEL_OPTIONS_FOR_PREPROCESSORS](),
    });

    let defaultEmberCLIBabelOptions;
    if (isExperimentEnabled('DELAYED_TRANSPILATION')) {
      defaultEmberCLIBabelOptions = {
        compileModules: false,
        disablePresetEnv: true,
        disableDebugTooling: true,
        disableEmberModulesAPIPolyfill: true,
      };
    } else {
      defaultEmberCLIBabelOptions = {
        compileModules: true,
      };
    }
    let emberCLIBabelConfigKey = this._emberCLIBabelConfigKey();
    this.__originalOptions[emberCLIBabelConfigKey] =
      this.options[emberCLIBabelConfigKey] =
      defaultsDeep(this.options[emberCLIBabelConfigKey], defaultEmberCLIBabelOptions);
  },

  /**
     Returns whether this is using a module unification format.
     @private
     @method isModuleUnification
     @return {Boolean} Whether this is using a module unification format.
  */
  isModuleUnification() {
    return false;
  },

  /**
   * Find an addon of the current addon.
   *
   * Example: ember-data depends on ember-cli-babel and wishes to have
   * additional control over transpilation this method helps.
   *
   * ```js
   * // ember-data/index.js
   * treeForAddon(tree) {
   *   let babel = this.findOwnAddonByName('ember-cli-babel');
   *
   *   return babel.transpileTree(tree, {
   *     // customize the babel step (see: ember-cli-addons readme for more details);
   *   });
   * }
   * ```
   *
   * @public
   * @method findOwnAddonByName
   */
  findOwnAddonByName(name) {
    return this.addons.find(addon => addon.name === name);
  },

  /**
   * Check if the current addon intends to be hinted. Typically this is for
   * hinting/linting libraries such as eslint or jshint
   *
   * @public
   * @method hintingEnabled
   */
  hintingEnabled() {
    let isProduction = process.env.EMBER_ENV === 'production';
    let testsEnabledDefault = process.env.EMBER_CLI_TEST_COMMAND || !isProduction;
    let explicitlyDisabled = this.app && this.app.options && this.app.options.hinting === false;

    return testsEnabledDefault && !explicitlyDisabled;
  },

  /**
    Shorthand method for [broccoli-concat](https://github.com/ember-cli/broccoli-concat)

    @private
    @method concatFiles
    @param {tree} tree Tree of files
    @param {Object} options Options for broccoli-concat
    @return {tree} Modified tree
  */
  concatFiles(tree, options) {
    options.sourceMapConfig = this.app.options.sourcemaps;
    return require('broccoli-concat')(tree, options);
  },

  /**
    Allows to mark the addon as developing, triggering live-reload in the project the addon is linked to.

    #### Uses:

    - Working on projects with internal addons

    @public
    @method isDevelopingAddon
    @return {Boolean}
  */
  isDevelopingAddon() {
    if (process.env.EMBER_ADDON_ENV === 'development' && (this.parent instanceof Project)) {
      const parentName = this.parent.name();
      // If the name in package.json and index.js match, we're definitely developing
      if (parentName === this.name) {
        return true;
      }

      // Addon names in index.js and package.json should be the same at all times whether they have scope or not.
      if (this.root === this.parent.root) {
        if (parentName !== this.name && !process.env.EMBER_CLI_IGNORE_ADDON_NAME_MISMATCH) {
          let pathToDisplay = process.cwd() === this.root ? process.cwd() : path.relative(process.cwd(), this.root);

          throw new SilentError(
            'Your names in package.json and index.js should match. ' +
            `The addon in ${pathToDisplay} currently have '${parentName}' in package.json and '${this.name}' in index.js. ` +
            'Until ember-cli v3.9, this error can be disabled by setting env variable EMBER_CLI_IGNORE_ADDON_NAME_MISMATCH to "true". ' +
            'For more information about this workaround, see: https://github.com/ember-cli/ember-cli/pull/7950.');
        }

        return true;
      }
    }
    return false;
  },

  /**
   * Discovers all child addons of this addon and an AddonInfo about
   * each addon in this.addonPackages (keyed on addon name).
   *
   * Child addons include those from 'dependencies' (not devDependencies)
   * and in-repo addons
   *
   * Any packageInfos that we find that are marked as not valid are excluded.
   *
   * @private
   * @method discoverAddons
   */
  discoverAddons() {
    let pkgInfo = this.packageInfoCache.getEntry(this.root);

    if (pkgInfo) {
      let addonPackageList = pkgInfo.discoverAddonAddons();
      this.addonPackages =
          pkgInfo.generateAddonPackages(addonPackageList,
            addonInfo => (this.shouldIncludeChildAddon && !this.shouldIncludeChildAddon(addonInfo)));

      // in case any child addons are invalid, dump to the console about them.
      pkgInfo.dumpInvalidAddonPackages(addonPackageList);
    } else {
      // There are cases where an addon can be created in memory and not actually
      // have a root entry (or have one that is not actually pointing to a directory,
      // like 'foo' in some of the tests. We don't want to crash, but want to let
      // things continue even though they're empty.
      this.addonPackages = Object.create(null);
    }
  },

  initializeAddons() {
    if (this._addonsInitialized) {
      return;
    }
    this._addonsInitialized = true;

    logger.info('initializeAddons for: %s', this.name);

    this.discoverAddons();

    this.addons = instantiateAddons(this, this.project, this.addonPackages);
    this.addons.forEach(addon => logger.info('addon: %s', addon.name));
  },

  /**
    Invoke the specified method for each enabled addon.

    @private
    @method eachAddonInvoke
    @param {String} methodName the method to invoke on each addon
    @param {Array} args the arguments to pass to the invoked method
  */
  eachAddonInvoke(methodName, args) {
    this.initializeAddons();

    let invokeArguments = args || [];

    return this.addons.reduce((sum, addon) => {
      let method = addon[methodName];
      if (method) {
        let val = method.apply(addon, invokeArguments);
        if (val) { sum.push(val); }
      }
      return sum;
    }, []);
  },

  /**
    Invoke the specified method for each of the project's addons.

    @private
    @method _eachProjectAddonInvoke
    @param {String} methodName the method to invoke on each addon
    @param {Array} args the arguments to pass to the invoked method
  */
  _eachProjectAddonInvoke(methodName, args) {
    this.initializeAddons();

    let invokeArguments = args || [];

    return this.project.addons.reduce((sum, addon) => {
      let method = addon[methodName];
      if (method) {
        let val = method.apply(addon, invokeArguments);
        if (val) { sum.push(val); }
      }
      return sum;
    }, []);
  },

  _addonPreprocessTree(type, tree) {
    return addonProcessTree(this, 'preprocessTree', type, tree);
  },

  _addonPostprocessTree(type, tree) {
    return addonProcessTree(this, 'postprocessTree', type, tree);
  },

  /**
    Generates a tree for the specified path

    @private
    @method treeGenerator
    @return {tree}
  */
  treeGenerator(dir) {
    let tree;

    if (!this.project) {
      this._warn(`Addon: \`${this.name}\` is missing addon.project, this may be the result of an addon forgetting to invoke \`super\` in its init.`);
    }
    // TODO: fix law of demeter `_watchmanInfo.canNestRoots` is obviously a poor idea
    if ((this.project && this.project._watchmanInfo.canNestRoots) ||
        this.isDevelopingAddon()) {
      const WatchedDir = require('broccoli-source').WatchedDir;
      tree = new WatchedDir(dir);
    } else {
      const UnwatchedDir = require('broccoli-source').UnwatchedDir;
      tree = new UnwatchedDir(dir);
    }

    return tree;
  },

  _treePathFor(treeName) {
    let treePath = this.treePaths[treeName];
    let absoluteTreePath = path.join(this.root, treePath);
    let normalizedAbsoluteTreePath = path.normalize(absoluteTreePath);

    return ensurePosixPath(normalizedAbsoluteTreePath);
  },

  /**
   * @private
   * @method _warn
   */
  _warn: warn,

  _emberCLIBabelConfigKey() {
    let emberCLIBabelInstance = findAddonByName(this.addons, 'ember-cli-babel');

    return emberCLIBabelConfigKey(emberCLIBabelInstance);
  },

  /**
    Returns a given type of tree (if present), merged with the
    application tree. For each of the trees available using this
    method, you can also use a direct method called `treeFor[Type]` (eg. `treeForApp`).

    Available tree names:
    - {{#crossLink "Addon/treeForApp:method"}}app{{/crossLink}}
    - {{#crossLink "Addon/treeForStyles:method"}}styles{{/crossLink}}
    - {{#crossLink "Addon/treeForTemplates:method"}}templates{{/crossLink}}
    - {{#crossLink "Addon/treeForAddonTemplates:method"}}addon-templates{{/crossLink}}
    - {{#crossLink "Addon/treeForAddon:method"}}addon{{/crossLink}}
    - {{#crossLink "Addon/treeForVendor:method"}}vendor{{/crossLink}}
    - {{#crossLink "Addon/treeForTestSupport:method"}}test-support{{/crossLink}}
    - {{#crossLink "Addon/treeForAddonTestSupport:method"}}addon-test-support{{/crossLink}}
    - {{#crossLink "Addon/treeForPublic:method"}}public{{/crossLink}}

    #### Uses:

    - manipulating trees at build time

    @public
    @method treeFor
    @param {String} name
    @return {Tree}
  */
  treeFor(treeType) {
    let node = heimdall.start({
      name: `treeFor(${this.name} - ${treeType})`,
      addonName: this.name,
      treeType,
      treeFor: true,
    });

    let cacheKeyForTreeType = this.cacheKeyForTree(treeType);

    let cachedTree = ADDON_TREE_CACHE.getItem(cacheKeyForTreeType);
    if (cachedTree) {
      node.stop();
      return cachedTree;
    }

    let trees = this.eachAddonInvoke('treeFor', [treeType]);
    let tree = this._treeFor(treeType);

    if (tree) {
      trees.push(tree);
    }

    if (this.isDevelopingAddon() && this.hintingEnabled() && treeType === 'app') {
      trees.push(this.jshintAddonTree());
    }

    let mergedTreesForType = mergeTrees(trees, {
      overwrite: true,
      annotation: `Addon#treeFor (${this.name} - ${treeType})`,
    });

    if (cacheKeyForTreeType) {
      ADDON_TREE_CACHE.setItem(cacheKeyForTreeType, mergedTreesForType);
    }

    node.stop();

    return mergedTreesForType;
  },

  /**
    @private
    @param {String} name
    @method _treeFor
    @return {tree}
  */
  _treeFor(name) {
    let treePath = path.resolve(this.root, this.treePaths[name]);
    let treeForMethod = this.treeForMethods[name];
    let tree;

    if (fs.existsSync(treePath)) {
      tree = this.treeGenerator(treePath);
    }

    if (this[treeForMethod]) {
      tree = this[treeForMethod](tree);
    }

    return tree;
  },

  /**
    Calculates a cacheKey for the given treeType. It is expected to return a
    cache key allowing multiple builds of the same tree to simply return the
    original tree (preventing duplicate work). If it returns null / undefined
    the tree in question will opt out of this caching system.

    This method is invoked prior to calling treeFor with the same tree name.

    You should override this method if you implement custom treeFor or treeFor*
    methods, which cause addons to opt-out of this caching.

    @public
    @method cacheKeyForTree
    @param {String} treeType
    @return {String} cacheKey
  */
  cacheKeyForTree(treeType) {
    let methodsToValidate = methodsForTreeType(treeType);
    let cacheKeyStats = heimdall.statsFor('cache-key-for-tree');

    // determine if treeFor* (or other methods for tree type) overrides for the given tree
    let modifiedMethods = methodsToValidate.filter(methodName => this[methodName] !== addonProto[methodName]);

    if (modifiedMethods.length) {
      cacheKeyStats.modifiedMethods++;
      cacheKeyLogger.info(`Opting out due to: modified methods: ${modifiedMethods.join(', ')}`);
      return null; // uncacheable
    }

    // determine if treeForMethods overrides for given tree
    if (this.treeForMethods[treeType] !== DEFAULT_TREE_FOR_METHODS[treeType]) {
      cacheKeyStats.treeForMethodsOverride++;
      cacheKeyLogger.info('Opting out due to: treeForMethods override');
      return null; // uncacheable
    }

    // compute cache key
    let cacheKey = calculateCacheKeyForTree(treeType, this);

    return cacheKey; // profit?
  },

  /**
    This method climbs up the hierarchy of addons
    up to the host application.

    This prevents previous addons (prior to `this.import`, ca 2.7.0)
    to break at importing assets when they are used nested in other addons.

    @private
    @method _findHost
  */
  _findHost() {
    let current = this;
    let app;

    // Keep iterating upward until we don't have a grandparent.
    // Has to do this grandparent check because at some point we hit the project.
    do {
      app = current.app || app;
    } while (current.parent.parent && (current = current.parent));

    return app;
  },

  /**
    This method is called when the addon is included in a build. You
    would typically use this hook to perform additional imports

    #### Uses:

    - including vendor files
    - setting configuration options

    *Note:* Any options set in the consuming application will override the addon.

    @public
    @method included
    @param {EmberApp|EmberAddon} parent The parent object which included this addon

    @example
    ```js
    included(parent) {
      this._super.included.apply(this, arguments);
      this.import(somePath);
    }
    ```
  */
  included(/* parent */) {
    if (!this._addonsInitialized) {
      // someone called `this._super.included` without `apply` (because of older
      // core-object issues that prevent a "real" super call from working properly)
      return;
    }

    this.eachAddonInvoke('included', [this]);
  },

  /**
     Imports an asset into this addon.

     @public
     @method import
     @param {Object|String} asset Either a path to the asset or an object with environment names and paths as key-value pairs.
     @param {Object} [options] Options object
     @param {String} [options.type] Either 'vendor' or 'test', defaults to 'vendor'
     @param {Boolean} [options.prepend] Whether or not this asset should be prepended, defaults to false
     @param {String} [options.destDir] Destination directory, defaults to the name of the directory the asset is in
     @since 2.7.0
   */
  import(asset, options) {
    options = options || {};
    options.resolveFrom = options.resolveFrom || this.root;

    let app = this._findHost();
    app.import(asset, options);
  },

  /**
    Returns the tree for all app files

    @public
    @method treeForApp
    @param {Tree} tree
    @return {Tree} App file tree
  */
  treeForApp(tree) {
    if (!isExperimentEnabled('MODULE_UNIFICATION')) {
      return tree;
    } else if (this.project.isModuleUnification() && this.isModuleUnification()) {
      return null;
    }

    let srcTreePath = path.resolve(this.root, this.treePaths.src);

    if (fs.existsSync(srcTreePath)) {
      const MUReexporter = require('broccoli-module-unification-reexporter');

      let srcTree = this.treeGenerator(srcTreePath);

      let reexportedOutput = new MUReexporter(srcTree, {
        namespace: this.name,
      });

      return reexportedOutput;
    }

    return tree;
  },

  /**
    Returns the tree for all template files

    @public
    @method treeForTemplates
    @param {Tree} tree
    @return {Tree} Template file tree
  */
  treeForTemplates(tree) {
    return tree;
  },

  /**
    @private
    @method treeForSrc
    @return
  */
  treeForSrc(rawSrcTree) {
    if (!isExperimentEnabled('MODULE_UNIFICATION')) {
      return null;
    }
    if (!rawSrcTree) { return null; }

    let srcNamespacedTree = new Funnel(rawSrcTree, {
      destDir: `${this.name}/src`,
      annotation: `Addon#treeForSrc(${this.name}) namespace`,
    });

    let srcAfterPreprocessTreeHook = this._addonPreprocessTree('src', srcNamespacedTree);

    let srcAfterTemplatePreprocessing = srcAfterPreprocessTreeHook;
    if (!isExperimentEnabled('DELAYED_TRANSPILATION') || registryHasPreprocessor(this.registry, 'template')) {
      srcAfterTemplatePreprocessing = preprocessTemplates(srcAfterPreprocessTreeHook, {
        registry: this.registry,
        annotation: `Addon#treeForSrc(${this.name})`,
      });
    }

    let srcAfterJsPreprocessing = preprocessJs(srcAfterTemplatePreprocessing, '/', this.name, {
      registry: this.registry,
    });

    return this._addonPostprocessTree('src', srcAfterJsPreprocessing);
  },

  /**
    Returns the tree for this addon's templates

    @public
    @method treeForAddonTemplates
    @param {Tree} tree
    @return {Tree} Addon Template file tree
  */
  treeForAddonTemplates(tree) {
    return tree;
  },

  /**
    Returns a tree for this addon

    @public
    @method treeForAddon
    @param {Tree} tree
    @return {Tree} Addon file tree

    @example
    ```js
    treeForAddon() {
      var tree = this._super.treeForAddon.apply(this, arguments);
      var checker = new VersionChecker(this);
      var isOldEmber = checker.for('ember', 'bower').lt('1.13.0');

      if (isOldEmber) {
        tree = new Funnel(tree, { exclude: [ /instance-initializers/ ] });
      }

      return tree;
    }
    ```
   */
  treeForAddon(tree) {
    if (!tree) {
      return tree;
    }

    let addonTree = this.compileAddon(tree);
    let stylesTree = this.compileStyles(this._treeFor('addon-styles'));

    return mergeTrees([addonTree, stylesTree], {
      annotation: `Addon#treeForAddon(${this.name})`,
    });
  },

  /**
    Returns the tree for all style files

    @public
    @method treeForStyles
    @param {Tree} tree The tree to process, usually `app/styles/` in the addon.
    @return {Tree} The return tree has the same contents as the input tree, but is moved so that the `app/styles/` path is preserved.
  */
  treeForStyles(tree) {
    if (!tree) {
      return tree;
    }

    return new Funnel(tree, {
      destDir: 'app/styles',
      annotation: `Addon#treeForStyles (${this.name})`,
    });
  },

  /**
    Returns the tree for all vendor files

    @public
    @method treeForVendor
    @param {Tree} tree
    @return {Tree} Vendor file tree
  */
  treeForVendor(tree) {
    return tree;
  },

  /**
    Returns the tree for all test support files

    @public
    @method treeForTestSupport
    @param {Tree} tree
    @return {Tree} Test Support file tree
  */
  treeForTestSupport(tree) {
    return tree;
  },

  /**
    Returns the tree for all public files

    @public
    @method treeForPublic
    @param {Tree} tree
    @return {Tree} Public file tree
  */
  treeForPublic(tree) {
    if (!tree) {
      return tree;
    }

    return new Funnel(tree, {
      srcDir: '/',
      destDir: `/${this.moduleName()}`,
      annotation: `Addon#treeForPublic (${this.name})`,
    });
  },

  /**
   Returns the tree for all test files namespaced to a given addon.

   @public
   @method treeForAddonTestSupport
   @param {Tree} tree
   @return {Tree}
   */
  treeForAddonTestSupport(tree) {
    if (!tree) {
      return tree;
    }

    let namespacedTree = new Funnel(tree, {
      srcDir: '/',
      destDir: `/${this.moduleName()}/test-support`,
      annotation: `Addon#treeForTestSupport (${this.name})`,
    });

    if (registryHasPreprocessor(this.registry, 'js')) {
      return this.preprocessJs(namespacedTree, '/', this.name, {
        registry: this.registry,
      });
    } else {
      this._warn(`Addon test support files were detected in \`${this._treePathFor('addon-test-support')}\`, but no JavaScript ` +
                 `preprocessors were found for \`${this.name}\`. Please make sure to add a preprocessor ` +
                 `(most likely \`ember-cli-babel\`) to \`dependencies\` (NOT \`devDependencies\`) in ` +
                 `\`${this.name}\`'s \`package.json\`.`);

      return processModulesOnly(namespacedTree, `Babel Fallback - Addon#treeForAddonTestSupport (${this.name})`);
    }
  },

  /**
    Runs the styles tree through preprocessors.

    @private
    @method compileStyles
    @param {Tree} addonStylesTree Styles file tree
    @return {Tree} Compiled styles tree
  */
  compileStyles(addonStylesTree) {
    if (addonStylesTree) {
      let preprocessedStylesTree = this._addonPreprocessTree('css', addonStylesTree);

      let processedStylesTree = preprocessCss(preprocessedStylesTree, '/', '/', {
        outputPaths: { 'addon': `${this.name}.css` },
        registry: this.registry,
      });

      return this._addonPostprocessTree('css', processedStylesTree);
    }
  },

  /**
    Looks in the addon/ and addon/templates trees to determine if template files
    exists that need to be precompiled.

    This is executed once when building, but not on rebuilds.

    @private
    @method shouldCompileTemplates
    @return {Boolean} indicates if templates need to be compiled for this addon
  */
  shouldCompileTemplates() {
    return this._fileSystemInfo().hasTemplates;
  },

  /**
     Looks in the addon/ and addon/templates trees to determine if template files
     exists in the pods format that need to be precompiled.

     This is executed once when building, but not on rebuilds.

     @private
     @method _shouldCompilePodTemplates
     @return {Boolean} indicates if pod based templates need to be compiled for this addon
  */
  _shouldCompilePodTemplates() {
    return this._fileSystemInfo().hasPodTemplates;
  },

  _fileSystemInfo() {
    if (this._cachedFileSystemInfo) {
      return this._cachedFileSystemInfo;
    }

    let jsExtensions = this.registry.extensionsForType('js');
    let templateExtensions = this.registry.extensionsForType('template');
    let addonTreePath = this._treePathFor('addon');
    let addonTemplatesTreePath = this._treePathFor('addon-templates');
    let addonTemplatesTreeInAddonTree = addonTemplatesTreePath.indexOf(addonTreePath) === 0;

    let files = this._getAddonTreeFiles();

    let addonTemplatesRelativeToAddonPath = addonTemplatesTreeInAddonTree && addonTemplatesTreePath.replace(`${addonTreePath}/`, '');
    let podTemplateMatcher = new RegExp(`template.(${templateExtensions.join('|')})$`);
    let hasPodTemplates = files.some(file => {
      // short circuit if this is actually a `addon/templates` file
      if (addonTemplatesTreeInAddonTree && file.indexOf(addonTemplatesRelativeToAddonPath) === 0) {
        return false;
      }

      return podTemplateMatcher.test(file);
    });

    let jsMatcher = new RegExp(`(${jsExtensions.join('|')})$`);
    let hasJSFiles = files.some(file => jsMatcher.test(file));

    if (!addonTemplatesTreeInAddonTree) {
      files = files.concat(this._getAddonTemplatesTreeFiles());
    }

    let extensionMatcher = new RegExp(`(${templateExtensions.join('|')})$`);
    let hasTemplates = files.some(file => extensionMatcher.test(file));

    this._cachedFileSystemInfo = {
      hasJSFiles,
      hasTemplates,
      hasPodTemplates,
    };

    return this._cachedFileSystemInfo;
  },

  _getAddonTreeFiles() {
    let addonTreePath = this._treePathFor('addon');

    if (fs.existsSync(addonTreePath)) {
      return walkSync(addonTreePath);
    }

    return [];
  },

  _getAddonTemplatesTreeFiles() {
    let addonTemplatesTreePath = this._treePathFor('addon-templates');

    if (fs.existsSync(addonTemplatesTreePath)) {
      return walkSync(addonTemplatesTreePath);
    }

    return [];
  },

  _addonTemplateFiles(addonTree) {
    if (this._cachedAddonTemplateFiles) {
      return this._cachedAddonTemplateFiles;
    }

    let trees = [];
    let addonTemplates = this._treeFor('addon-templates');
    let standardTemplates;

    if (addonTemplates) {
      standardTemplates = new Funnel(addonTemplates, {
        srcDir: '/',
        destDir: `${this.name}/templates`,
        annotation: `Addon#_addonTemplateFiles (${this.name})`,
      });

      trees.push(standardTemplates);
    }

    if (this._shouldCompilePodTemplates()) {
      let includePatterns = this.registry.extensionsForType('template')
        .map(extension => `**/*/template.${extension}`);

      let podTemplates = new Funnel(addonTree, {
        include: includePatterns,
        destDir: `${this.name}/`,
        annotation: 'Funnel: Addon Pod Templates',
      });

      trees.push(podTemplates);
    }

    this._cachedAddonTemplateFiles = mergeTrees(trees, {
      annotation: `TreeMerge (${this.name} templates)`,
    });

    return this._cachedAddonTemplateFiles;
  },

  /**
    Runs the templates tree through preprocessors.

    @private
    @method compileTemplates
    @param {Tree} tree Templates file tree
    @return {Tree} Compiled templates tree
  */
  compileTemplates(addonTree) {
    if (this.shouldCompileTemplates()) {
      if (!isExperimentEnabled('DELAYED_TRANSPILATION') && !registryHasPreprocessor(this.registry, 'template')) {
        throw new SilentError(`Addon templates were detected, but there are no template compilers registered for \`${this.name}\`. ` +
          `Please make sure your template precompiler (commonly \`ember-cli-htmlbars\`) is listed in \`dependencies\` ` +
          `(NOT \`devDependencies\`) in \`${this.name}\`'s \`package.json\`.`);
      }

      let preprocessedTemplateTree = this._addonPreprocessTree('template', this._addonTemplateFiles(addonTree));

      let processedTemplateTree;
      if (!isExperimentEnabled('DELAYED_TRANSPILATION') || registryHasPreprocessor(this.registry, 'template')) {
        processedTemplateTree = preprocessTemplates(preprocessedTemplateTree, {
          annotation: `compileTemplates(${this.name})`,
          registry: this.registry,
        });
      } else {
        processedTemplateTree = preprocessedTemplateTree;
      }

      let postprocessedTemplateTree = this._addonPostprocessTree('template', processedTemplateTree);

      return processModulesOnly(postprocessedTemplateTree, 'Babel: Modules for Templates');
    }
  },

  /**
    Runs the addon tree through preprocessors.

    @private
    @method compileAddon
    @param {Tree} tree Addon file tree
    @return {Tree} Compiled addon tree
  */
  compileAddon(tree) {
    if (!this.options) {
      this._warn(
        `Ember CLI addons manage their own module transpilation during the \`treeForAddon\` processing. ` +
        `\`${this.name}\` (found at \`${this.root}\`) has removed \`this.options\` ` +
        `which conflicts with the addons ability to transpile its \`addon/\` files properly. ` +
        `Falling back to default babel configuration options.`
      );

      this.options = {};
    }

    if (!this.options.babel) {
      this._warn(
        `Ember CLI addons manage their own module transpilation during the \`treeForAddon\` processing. ` +
        `\`${this.name}\` (found at \`${this.root}\`) has overridden the \`this.options.babel\` ` +
        `options which conflicts with the addons ability to transpile its \`addon/\` files properly. ` +
        `Falling back to default babel configuration options.`
      );

      this.options.babel = this.__originalOptions.babel;
    }

    let emberCLIBabelConfigKey = this._emberCLIBabelConfigKey();
    if (!this.options[emberCLIBabelConfigKey]) {
      this._warn(
        `Ember CLI addons manage their own module transpilation during the \`treeForAddon\` processing. ` +
        `\`${this.name}\` (found at \`${this.root}\`) has overridden the \`this.options.${emberCLIBabelConfigKey}\` ` +
        `options which conflicts with the addons ability to transpile its \`addon/\` files properly. ` +
        `Falling back to default babel configuration options.`
      );

      this.options[emberCLIBabelConfigKey] = this.__originalOptions[emberCLIBabelConfigKey];
    }

    let addonJs = this.processedAddonJsFiles(tree);
    let templatesTree = this.compileTemplates(tree);

    let trees = [addonJs, templatesTree].filter(Boolean);

    return mergeTrees(trees, {
      overwrite: true,
      annotation: `Addon#compileAddon(${this.name})`,
    });
  },

  /**
    Returns a tree with JSHhint output for all addon JS.

    @private
    @method jshintAddonTree
    @return {Tree} Tree with JShint output (tests)
  */
  jshintAddonTree() {
    let trees = [];

    let addonPath = this._treePathFor('addon');
    if (fs.existsSync(addonPath)) {
      let addonJs = new Funnel(this.addonJsFiles(addonPath), {
        srcDir: this.moduleName(),
        destDir: 'addon',
        allowEmpty: true,
      });
      let lintAddonJsTrees = this._eachProjectAddonInvoke('lintTree', ['addon', addonJs]);

      let addonTemplates = new Funnel(this._addonTemplateFiles(addonPath), {
        srcDir: `${this.moduleName()}/templates`,
        destDir: 'addon/templates',
        allowEmpty: true,
      });
      let lintTemplateTrees = this._eachProjectAddonInvoke('lintTree', ['templates', addonTemplates]);

      trees = trees.concat(lintAddonJsTrees, lintTemplateTrees);
    }

    let addonTestSupportPath = this._treePathFor('addon-test-support');
    if (fs.existsSync(addonTestSupportPath)) {
      let addonTestSupportTree = new Funnel(addonTestSupportPath, { destDir: 'addon-test-support' });
      let lintAddonTestSupportJsTrees = this._eachProjectAddonInvoke('lintTree', ['addon-test-support', addonTestSupportTree]);
      trees = trees.concat(lintAddonTestSupportJsTrees);
    }

    let appPath = this._treePathFor('app');
    if (fs.existsSync(appPath)) {
      let appTree = new Funnel(appPath, { destDir: 'app' });
      let lintAppJsTrees = this._eachProjectAddonInvoke('lintTree', ['app', appTree]);
      trees = trees.concat(lintAppJsTrees);
    }

    let testSupportPath = this._treePathFor('test-support');
    if (fs.existsSync(testSupportPath)) {
      let testSupportTree = new Funnel(testSupportPath, { destDir: 'test-support' });
      let lintTestSupportJsTrees = this._eachProjectAddonInvoke('lintTree', ['test-support', testSupportTree]);
      trees = trees.concat(lintTestSupportJsTrees);
    }

    let lintTrees = trees.filter(Boolean);
    let lintedAddon = mergeTrees(lintTrees, {
      overwrite: true,
      annotation: 'TreeMerger (addon-lint)',
    });

    return new Funnel(lintedAddon, {
      srcDir: '/',
      destDir: `${this.name}/tests/`,
      annotation: `Funnel: Addon#jshintAddonTree(${this.name})`,
    });
  },

  /**
    Returns a tree containing the addon's js files

    @private
    @method addonJsFiles
    @return {Tree} The filtered addon js files
  */
  addonJsFiles(tree) {
    let includePatterns = this.registry.extensionsForType('js')
      .map(extension => new RegExp(`${extension}$`));

    return new Funnel(tree, {
      include: includePatterns,
      destDir: this.moduleName(),
      annotation: 'Funnel: Addon JS',
    });
  },


  /**
    Preprocesses a javascript tree.

    @private
    @method preprocessJs
    @return {Tree} Preprocessed javascript
  */
  preprocessJs() {
    return preprocessJs.apply(preprocessJs, arguments);
  },

  /**
    Returns a tree with all javascript for this addon.

    @private
    @method processedAddonJsFiles
    @param {Tree} the tree to preprocess
    @return {Tree} Processed javascript file tree
  */
  processedAddonJsFiles(addonTree) {
    let preprocessedAddonJS = this._addonPreprocessTree('js', this.addonJsFiles(addonTree));

    let processedAddonJS = this.preprocessJs(preprocessedAddonJS, '/', this.name, {
      annotation: `processedAddonJsFiles(${this.name})`,
      registry: this.registry,
    });

    let postprocessedAddonJs = this._addonPostprocessTree('js', processedAddonJS);

    if (!registryHasPreprocessor(this.registry, 'js')) {
      this._warn(`Addon files were detected in \`${this._treePathFor('addon')}\`, but no JavaScript ` +
                 `preprocessors were found for \`${this.name}\`. Please make sure to add a preprocessor ` +
                 '(most likely `ember-cli-babel`) to in `dependencies` (NOT `devDependencies`) in ' +
                 `\`${this.name}\`'s \`package.json\`.`);

      postprocessedAddonJs = processModulesOnly(
        postprocessedAddonJs,
        `Babel Fallback - Addon#processedAddonJsFiles(${this.name})`
      );
    }

    return postprocessedAddonJs;
  },

  /**
    Returns the module name for this addon.

    @public
    @method moduleName
    @return {String} module name
  */
  moduleName() {
    if (!this.modulePrefix) {
      this.modulePrefix = (this.modulePrefix || this.name).toLowerCase().replace(/\s/g, '-');
    }

    return this.modulePrefix;
  },

  /**
    Returns the path for addon blueprints.

    @public
    @method blueprintsPath
    @return {String} The path for blueprints

    @example
    - [ember-cli-coffeescript](https://github.com/kimroen/ember-cli-coffeescript/blob/v1.13.2/index.js#L26)
  */
  blueprintsPath() {
    let blueprintPath = path.join(this.root, 'blueprints');

    if (fs.existsSync(blueprintPath)) {
      return blueprintPath;
    }
  },

  /**
    Augments the applications configuration settings.

    Object returned from this hook is merged with the application's configuration object.

    Application's configuration always take precedence.

    #### Uses:

    - Modifying configuration options (see list of defaults [here](https://github.com/ember-cli/ember-cli/blob/v2.4.3/lib/broccoli/ember-app.js#L163))
      - For example
        - `minifyJS`
        - `storeConfigInMeta`
        - et, al

    @public
    @method config
    @param {String} env Name of current environment (ie "development")
    @param {Object} baseConfig Initial application configuration
    @return {Object} Configuration object to be merged with application configuration.

    @example
    ```js
    config(environment, appConfig) {
      return {
        someAddonDefault: "foo"
      };
    }
    ```
  */
  config(env, baseConfig) {
    let configPath = path.join(this.root, 'config', 'environment.js');

    if (fs.existsSync(configPath)) {
      const configGenerator = require(configPath);

      return configGenerator(env, baseConfig);
    }
  },

  /**
    @public
    @method dependencies
    @return {Object} The addon's dependencies based on the addon's package.json
  */
  dependencies() {
    let pkg = this.pkg;
    return pkg ? Object.assign({}, pkg.devDependencies, pkg.dependencies) : {};
  },

  /**
    @public
    @method isEnabled
    @return {Boolean} Whether or not this addon is enabled
  */
  isEnabled() {
    return true;
  },

  /**
    Can be used to exclude addons from being added as a child addon.

    #### Uses:

    - Abstract away multiple addons while only including one into the built assets

    @public
    @method shouldIncludeChildAddon
    @param {Addon} childAddon
    @return {Boolean} Whether or not a child addon is supposed to be included

    @example
    ```js
    shouldIncludeChildAddon(childAddon) {
      if(childAddon.name === 'ember-cli-some-legacy-select-component') {
        return this.options.legacyMode;
      } else if(childAddon.name === 'ember-cli-awesome-new-select-component') {
        return !this.options.legacyMode;
      } else {
        return this._super.shouldIncludeChildAddon.apply(this, arguments);
      }
    }
    ```
  */
  shouldIncludeChildAddon() {
    return true;
  },
};

// Methods without default implementation

/**
  Allows the specification of custom addon commands.
  Expects you to return an object whose key is the name of the command and value is the command instance..

  This function is not implemented by default

  #### Uses:

  - Include custom commands into consuming application

  @public
  @method includedCommands
  @return {Object} An object with included commands

  @example
  ```js
  includedCommands() {
    return {
      'do-foo': require('./lib/commands/foo')
    };
  }
  ```
*/


/**
  Allows addons to define a custom transfrom function that other addons and app can use when using `app.import`.

  This function is not implemented by default

  #### Uses:

  - An app or addons want to transform a dependency that is being imported using `app.import`.

  @public
  @method importTransforms
  @return {Object} An object with custom transforms

  @example
  ```js
  importTransforms() {
    return {
      'my-custom-transform': function(tree, options) {
        // transform the incoming tree and return the updated tree
      }
    };
  }
  ```

  Alternatively, if you want to process `options` before being passed into the custom transform function, use:

  @example
  ```js
  importTransforms() {
    return {
      'my-custom-transform': {
        transform: function(tree, options) {
            // transform the incoming tree and return the updated tree
        },
        processOptions: function(assetPath, entry, options) {
          // process your options

          return options
        }
    };
  }
  ```
*/

/**
  Pre-process a tree

  #### Uses:

  - removing / adding files from the build.

  @public
  @method preprocessTree
  @param {String} type What kind of tree (eg. 'js', 'css', 'src', 'template')
  @param {Tree} tree Tree to process
  @return {Tree} Processed tree
 */


/**
  Post-process a tree

  @public
  @method postprocessTree
  @param {String} type What kind of tree (eg. 'js', 'css', 'src', 'template')
  @param {Tree} tree Tree to process
  @return {Tree} Processed tree

  @example
  - [broccoli-asset-rev](https://github.com/rickharrison/broccoli-asset-rev/blob/c82c3580855554a31f7d6600b866aecf69cdaa6d/index.js#L29)
 */


/**
  This hook allows you to make changes to the express server run by ember-cli.

  It's passed a `startOptions` object which contains:
  - `app` Express server instance
  - `options` A hash with:
    - `project` Current {{#crossLink "Project"}}project{{/crossLink}}
    - `watcher`
    - `environment`

  This function is not implemented by default

  #### Uses:

  - Tacking on headers to each request
  - Modifying the request object

  *Note:* that this should only be used in development, and if you need the same behavior in production you'll
  need to configure your server.

  @public
  @method serverMiddleware
  @param {Object} startOptions Express server start options

  @example
  ```js
  serverMiddleware(startOptions) {
    var app = startOptions.app;

    app.use(function(req, res, next) {
      // Some middleware
    });
  }
  ```

  - [ember-cli-content-security-policy](https://github.com/rwjblue/ember-cli-content-security-policy/blob/v0.5.0/index.js#L84)
  - [history-support-addon](https://github.com/ember-cli/ember-cli/blob/v2.4.3/lib/tasks/server/middleware/history-support/index.js#L25)
 */


/**
 This hook allows you to make changes to the express server run by testem.

 This function is not implemented by default

 #### Uses:

 - Adding custom test-specific endpoints
 - Manipulating HTTP requests in tests

 @public
 @method testemMiddleware
 @param {Object} app the express app instance
 */


/**
  This hook is called before a build takes place.

  @public
  @method preBuild
  @param {Object} result Build object
*/


/**
  This hook is called after a build is complete.

  It's passed a `result` object which contains:
  - `directory` Path to build output

  #### Uses:

  - Slow tree listing
  - May be used to manipulate your project after build has happened

  @public
  @method postBuild
  @param {Object} result Build result object
*/


/**
  This hook is called after the build has been processed and the build files have been copied to the output directory

  It's passed a `result` object which contains:
  - `directory` Path to build output

  @public
  @method outputReady
  @param {Object} result Build result object

  @example
  - Opportunity to symlink or copy files elsewhere.
  - [ember-cli-rails-addon](https://github.com/rondale-sc/ember-cli-rails-addon/blob/v0.7.0/index.js#L45)
    - In this case we are using this in tandem with a rails middleware to remove a lock file.
      This allows our ruby gem to block incoming requests until after the build happens reliably.
 */


/**
  This hook is called when an error occurs during the preBuild, postBuild or outputReady hooks
  for addons, or when the build fails

  #### Uses:

  - Custom error handling during build process

  @public
  @method buildError
  @param {Error} error The error that was caught during the processes listed above

  @example
  - [ember-cli-rails-addon](https://github.com/rondale-sc/ember-cli-rails-addon/blob/v0.7.0/index.js#L11)
*/


/**
 Used to add preprocessors to the preprocessor registry. This is often used by addons like [ember-cli-htmlbars](https://github.com/ember-cli/ember-cli-htmlbars)
 and [ember-cli-coffeescript](https://github.com/kimroen/ember-cli-coffeescript) to add a `template` or `js` preprocessor to the registry.

 **Uses:**

 - Adding preprocessors to the registry.

 @public
 @method setupPreprocessorRegistry
 @param {String} type either `"self"` or `"parent"`
 @param registry the registry to be set up

 @example
 ```js
 setupPreprocessorRegistry(type, registry) {
   // ensure that broccoli-ember-hbs-template-compiler is not processing hbs files
   registry.remove('template', 'broccoli-ember-hbs-template-compiler');

   registry.add('template', {
     name: 'ember-cli-htmlbars',
     ext: 'hbs',
     _addon: this,
     toTree(tree) {
       var htmlbarsOptions = this._addon.htmlbarsOptions();
       return htmlbarsCompile(tree, htmlbarsOptions);
     },

     precompile(string) {
       var htmlbarsOptions = this._addon.htmlbarsOptions();
       var templateCompiler = htmlbarsOptions.templateCompiler;
       return utils.template(templateCompiler, string);
     }
   });

   if (type === 'parent') {
     this.parentRegistry = registry;
   }
 }
 ```
*/


/**
 Return value is merged into the **tests** tree. This lets you inject
 linter output as test results.

 **Uses:**

 - JSHint
 - any other form of automated test generation that turns code into tests

 @public
 @method lintTree
 @param {String} treeType `app`, `tests`, `templates`, or `addon`
 @param {Tree} tree tree of files (JavaScript files for `app`, `tests`, and `addon` types)

 @example
 - [ember-cli-qunit](https://github.com/ember-cli/ember-cli-qunit/blob/v1.4.1/index.js#L206)
 - [ember-cli-mocha](https://github.com/ef4/ember-cli-mocha/blob/66803037fe203b24e96dea83a2bd91de48b842e1/index.js#L101)
*/


/**
 Allow addons to implement contentFor method to add string output into the associated `{{content-for 'foo'}}` section in `index.html`

 **Uses:**

 - For instance, to inject analytics code into `index.html`

 @public
 @method contentFor
 @param type
 @param config
 @param content

 @example
 - [ember-cli-google-analytics](https://github.com/pgrippi/ember-cli-google-analytics/blob/v1.5.0/index.js#L79)
*/

function methodsForTreeType(treeType) {
  let treeMethods = DEFAULT_TREE_FOR_METHOD_METHODS[treeType];

  return GLOBAL_TREE_FOR_METHOD_METHODS.concat(treeMethods);
}

let Addon = CoreObject.extend(addonProto);

Addon.prototype[BUILD_BABEL_OPTIONS_FOR_PREPROCESSORS] = function() {
  let emberCLIBabelInstance = findAddonByName(this.addons, 'ember-cli-babel');
  let version;
  if (emberCLIBabelInstance) {
    version = require(path.join(emberCLIBabelInstance.root, 'package')).version;
  }

  if (version && semver.satisfies(version, '^5')) {
    return {
      modules: 'amdStrict',
      moduleIds: true,
      resolveModuleSource: require('amd-name-resolver').moduleResolve,
    };
  } else {
    return {};
  }
};

module.exports = Addon;
module.exports._resetTreeCache = _resetTreeCache;
module.exports._treeCache = ADDON_TREE_CACHE;
