'use strict';

const VersionChecker = require('ember-cli-version-checker');
const clone = require('clone');
const path = require('path');
const semver = require('semver');

const defaultShouldIncludeHelpers = require('./lib/default-should-include-helpers');
const findApp = require('./lib/find-app');

const APP_BABEL_RUNTIME_VERSION = new WeakMap();

let count = 0;

module.exports = {
  name: 'ember-cli-babel',
  configKey: 'ember-cli-babel',

  init() {
    this._super.init && this._super.init.apply(this, arguments);

    let checker = new VersionChecker(this);
    let dep = checker.for('ember-cli', 'npm');

    if (dep.lt('2.13.0')) {
      throw new Error(`ember-cli-babel@7 (used by ${this._parentName()} at ${this.parent.root}) cannot be used by ember-cli versions older than 2.13, you used ${dep.version}`);
    }
  },

  buildBabelOptions(_config) {
    let config = _config || this._getAddonOptions();

    return this._getBabelOptions(config);
  },

  _debugTree() {
    if (!this._cachedDebugTree) {
      this._cachedDebugTree = require('broccoli-debug').buildDebugCallback(`ember-cli-babel:${this._parentName()}`);
    }

    return this._cachedDebugTree.apply(null, arguments);
  },

  transpileTree(inputTree, config) {
    let description = `000${++count}`.slice(-3);
    let postDebugTree = this._debugTree(inputTree, `${description}:input`);

    let options = this.buildBabelOptions(config);
    let output;
    if (this._shouldDoNothing(options)) {
      output = postDebugTree;
    } else {
      let BabelTranspiler = require('broccoli-babel-transpiler');
      output = new BabelTranspiler(postDebugTree, options);
    }

    return this._debugTree(output, `${description}:output`);
  },

  setupPreprocessorRegistry(type, registry) {
    registry.add('js', {
      name: 'ember-cli-babel',
      ext: this._getExtensions(this._getAddonOptions()),
      toTree: (tree) => this.transpileTree(tree)
    });
  },

  _shouldIncludePolyfill() {
    let addonOptions = this._getAddonOptions();
    let customOptions = addonOptions['ember-cli-babel'];

    if (customOptions && 'includePolyfill' in customOptions) {
      return customOptions.includePolyfill === true;
    } else {
      return false;
    }
  },

  _importPolyfill(app) {
    let polyfillPath = 'vendor/babel-polyfill/polyfill.js';

    if (this.import) {  // support for ember-cli >= 2.7
      this.import(polyfillPath, { prepend: true });
    } else if (app.import) { // support ember-cli < 2.7
      app.import(polyfillPath, { prepend: true });
    } else {
      // eslint-disable-next-line no-console
      console.warn('Please run: ember install ember-cli-import-polyfill');
    }
  },

  _shouldIncludeHelpers(options) {
    let appOptions = this._getAppOptions();
    let customOptions = appOptions['ember-cli-babel'];

    let shouldIncludeHelpers = false;

    if (!this._shouldCompileModules(options)) {
      // we cannot use external helpers if we are not transpiling modules
      return false;
    } else if (customOptions && 'includeExternalHelpers' in customOptions) {
      shouldIncludeHelpers = customOptions.includeExternalHelpers === true;
    } else {
      // Check the project to see if we should include helpers based on heuristics.
      shouldIncludeHelpers = defaultShouldIncludeHelpers(this.project);
    }

    let appEmberCliBabelPackage = this.project.addons.find(a => a.name === 'ember-cli-babel').pkg;
    let appEmberCliBabelVersion = appEmberCliBabelPackage && appEmberCliBabelPackage.version;

    if (appEmberCliBabelVersion && semver.gte(appEmberCliBabelVersion, '7.3.0-beta.1')) {
      return shouldIncludeHelpers;
    } else if (shouldIncludeHelpers) {
      this.project.ui.writeWarnLine(
        `${this._parentName()} attempted to include external babel helpers to make your build size smaller, but your root app's ember-cli-babel version is not high enough. Please update ember-cli-babel to v7.3.0-beta.1 or later.`
      );
    }

    return false;
  },

  _getHelperVersion() {
    if (!APP_BABEL_RUNTIME_VERSION.has(this.project)) {
      let checker = new VersionChecker(this.project);
      APP_BABEL_RUNTIME_VERSION.set(this.project, checker.for('@babel/runtime', 'npm').version);
    }

    return APP_BABEL_RUNTIME_VERSION.get(this.project);
  },

  _getHelpersPlugin() {
    return [
      [
        require.resolve('@babel/plugin-transform-runtime'),
        {
          version: this._getHelperVersion(),
          regenerator: false,
          useESModules: true
        }
      ]
    ]
  },

  treeForAddon() {
    // Helpers are a global config, so only the root application should bother
    // generating and including the file.
    let isRootBabel = this.parent === this.project;
    let shouldIncludeHelpers = isRootBabel && this._shouldIncludeHelpers(this._getAppOptions());

    if (!shouldIncludeHelpers) { return; }

    const path = require('path');
    const Funnel = require('broccoli-funnel');
    const UnwatchedDir = require('broccoli-source').UnwatchedDir;

    const babelHelpersPath = path.dirname(require.resolve('@babel/runtime/package.json'));

    let babelHelpersTree = new Funnel(new UnwatchedDir(babelHelpersPath), {
      srcDir: 'helpers/esm',
      destDir: '@babel/runtime/helpers/esm'
    });

    return this.transpileTree(babelHelpersTree, {
      'ember-cli-babel': {
        // prevents the helpers from being double transpiled, and including themselves
        disablePresetEnv: true
      }
    });
  },

  treeForVendor() {
    if (!this._shouldIncludePolyfill()) return;

    const Funnel = require('broccoli-funnel');
    const UnwatchedDir = require('broccoli-source').UnwatchedDir;

    // Find babel-core's browser polyfill and use its directory as our vendor tree
    let polyfillDir = path.dirname(require.resolve('@babel/polyfill/dist/polyfill'));

    let polyfillTree = new Funnel(new UnwatchedDir(polyfillDir), {
      destDir: 'babel-polyfill'
    });

    return polyfillTree;
  },

  included: function(app) {
    this._super.included.apply(this, arguments);
    this.app = app;

    if (this._shouldIncludePolyfill()) {
      this._importPolyfill(app);
    }
  },

  isPluginRequired(pluginName) {
    let targets = this._getTargets();

    // if no targets are setup, assume that all plugins are required
    if (!targets) { return true; }

    const isPluginRequired = require('@babel/preset-env').isPluginRequired;
    const pluginList = require('@babel/preset-env/data/plugins');

    return isPluginRequired(targets, pluginList[pluginName]);
  },

  _getAddonOptions() {
    let parentOptions = this.parent && this.parent.options;
    let appOptions = this.app && this.app.options;

    if (parentOptions) {
      let customAddonOptions = parentOptions['ember-cli-babel'];

      if (customAddonOptions && 'includeExternalHelpers' in customAddonOptions) {
        throw new Error('includeExternalHelpers is not supported in addon configurations, it is an app-wide configuration option');
      }
    }

    return parentOptions || appOptions || {};
  },

  _getAppOptions() {
    let app = findApp(this);

    return (app && app.options) || {};
  },

  _parentName() {
    let parentName;

    if (this.parent) {
      if (typeof this.parent.name === 'function') {
        parentName = this.parent.name();
      } else {
        parentName = this.parent.name;
      }
    }

    return parentName;
  },

  _getAddonProvidedConfig(addonOptions) {
    let options = clone(addonOptions.babel || {});

    let plugins = options.plugins || [];
    let postTransformPlugins = options.postTransformPlugins || [];

    return {
      options,
      plugins,
      postTransformPlugins
    };
  },

  _getExtensions(config) {
    let emberCLIBabelConfig = config['ember-cli-babel'] || {};
    return emberCLIBabelConfig.extensions || ['js'];
  },

  _getBabelOptions(config) {
    let addonProvidedConfig = this._getAddonProvidedConfig(config);
    let shouldCompileModules = this._shouldCompileModules(config);
    let shouldIncludeHelpers = this._shouldIncludeHelpers(config);
    let shouldIncludeDecoratorPlugins = this._shouldIncludeDecoratorPlugins(config);

    let emberCLIBabelConfig = config['ember-cli-babel'];
    let shouldRunPresetEnv = true;
    let providedAnnotation;
    let throwUnlessParallelizable;

    if (emberCLIBabelConfig) {
      providedAnnotation = emberCLIBabelConfig.annotation;
      shouldRunPresetEnv = !emberCLIBabelConfig.disablePresetEnv;
      throwUnlessParallelizable = emberCLIBabelConfig.throwUnlessParallelizable;
    }

    let sourceMaps = false;
    if (config.babel && 'sourceMaps' in config.babel) {
      sourceMaps = config.babel.sourceMaps;
    }

    let filterExtensions = this._getExtensions(config);

    let options = {
      annotation: providedAnnotation || `Babel: ${this._parentName()}`,
      sourceMaps,
      throwUnlessParallelizable,
      filterExtensions
    };

    let userPlugins = addonProvidedConfig.plugins;
    let userPostTransformPlugins = addonProvidedConfig.postTransformPlugins;

    if (shouldIncludeDecoratorPlugins) {
      userPlugins = this._addDecoratorPlugins(userPlugins.slice());
    }

    options.plugins = [].concat(
      shouldIncludeHelpers && this._getHelpersPlugin(),
      userPlugins,
      this._getDebugMacroPlugins(config),
      this._getEmberModulesAPIPolyfill(config),
      shouldCompileModules && this._getModulesPlugin(),
      userPostTransformPlugins
    ).filter(Boolean);

    options.presets = [
      shouldRunPresetEnv && this._getPresetEnv(addonProvidedConfig),
    ].filter(Boolean);

    if (shouldCompileModules) {
      options.moduleIds = true;
      options.getModuleId = require('./lib/relative-module-paths').getRelativeModulePath;
    }

    options.highlightCode = false;
    options.babelrc = false;

    return options;
  },

  _shouldIncludeDecoratorPlugins(config) {
    let customOptions = config['ember-cli-babel'] || {};

    return customOptions.disableDecoratorTransforms !== true;
  },

  _addDecoratorPlugins(plugins) {
    const { hasPlugin, addPlugin } = require('ember-cli-babel-plugin-helpers');

    if (hasPlugin(plugins, '@babel/plugin-proposal-decorators')) {
      if (this.parent === this.project) {
        this.project.ui.writeWarnLine(`${
          this._parentName()
        } has added the decorators plugin to its build, but ember-cli-babel provides these by default now! You can remove the transforms, or the addon that provided them, such as @ember-decorators/babel-transforms. Ember supports the stage 1 decorator spec and transforms, so if you were using stage 2, you'll need to ensure that your decorators are compatible, or convert them to stage 1.`);
      }
    } else {
      addPlugin(
        plugins, 
        [require.resolve('@babel/plugin-proposal-decorators'), { legacy: true }], 
        {
          before: ['@babel/plugin-proposal-class-properties', '@babel/plugin-transform-typescript']
        }
      );
    }


    if (hasPlugin(plugins, '@babel/plugin-proposal-class-properties')) {
      if (this.parent === this.project) {
        this.project.ui.writeWarnLine(`${
          this._parentName()
        } has added the class-properties plugin to its build, but ember-cli-babel provides these by default now! You can remove the transforms, or the addon that provided them, such as @ember-decorators/babel-transforms.`);
      }
    } else {
      addPlugin(
        plugins, 
        [require.resolve('@babel/plugin-proposal-class-properties'), { loose: true }], 
        {
          after: ['@babel/plugin-proposal-decorators'],
          before: ['@babel/plugin-transform-typescript']
        }
      );
    }

    if (hasPlugin(plugins, 'babel-plugin-filter-imports')) {
      let checker = new VersionChecker(this.parent).for('babel-plugin-filter-imports', 'npm');

      if (checker.lt('3.0.0')) {
        addPlugin(
          plugins, 
          require.resolve('./lib/dedupe-internal-decorators-plugin'),
          {
            after: ['babel-plugin-filter-imports']
          }
        );
      }
    }

    return plugins;
  },

  _getDebugMacroPlugins(config) {
    let addonOptions = config['ember-cli-babel'] || {};

    if (addonOptions.disableDebugTooling) { return; }

    const isProduction = process.env.EMBER_ENV === 'production';
    const isDebug = !isProduction;

    let options = {
      flags: [
        {
          source: '@glimmer/env',
          flags: { DEBUG: isDebug, CI: !!process.env.CI }
        }
      ],

      externalizeHelpers: {
        global: 'Ember'
      },

      debugTools: {
        isDebug,
        source: '@ember/debug',
        assertPredicateIndex: 1
      }
    };

    return [[require.resolve('babel-plugin-debug-macros'), options]];
  },

  _getEmberModulesAPIPolyfill(config) {
    let addonOptions = config['ember-cli-babel'] || {};

    if (addonOptions.disableEmberModulesAPIPolyfill) { return; }

    if (this._emberVersionRequiresModulesAPIPolyfill()) {
      const blacklist = this._getEmberModulesAPIBlacklist();

      return [[require.resolve('babel-plugin-ember-modules-api-polyfill'), { blacklist }]];
    }
  },

  _getPresetEnv(config) {
    let options = config.options;

    let targets = this.project && this.project.targets;
    let presetOptions = Object.assign({}, options, {
      modules: false,
      targets
    });

    // delete any properties added to `options.babel` that
    // are invalid for @babel/preset-env
    delete presetOptions.sourceMaps;
    delete presetOptions.plugins;
    delete presetOptions.postTransformPlugins;

    return [require.resolve('@babel/preset-env'), presetOptions];
  },

  _getTargets() {
    let targets = this.project && this.project.targets;

    let parser = require('@babel/preset-env/lib/targets-parser').default;
    if (typeof targets === 'object' && targets !== null) {
      return parser(targets);
    } else {
      return targets;
    }
  },

  _getModulesPlugin() {
    const resolvePath = require('./lib/relative-module-paths').resolveRelativeModulePath;

    return [
      [require.resolve('babel-plugin-module-resolver'), { resolvePath }],
      [require.resolve('@babel/plugin-transform-modules-amd'), { noInterop: true }],
    ];
  },

  /*
   * Used to discover if the addon's current configuration will compile modules
   * or not.
   *
   * @public
   * @method shouldCompileModules
   */
  shouldCompileModules() {
    return this._shouldCompileModules(this._getAddonOptions());
  },

  // will use any provided configuration
  _shouldCompileModules(options) {
    let addonOptions = options['ember-cli-babel'];

    if (addonOptions && 'compileModules' in addonOptions) {
      return addonOptions.compileModules;
    } else {
      return semver.gt(this.project.emberCLIVersion(), '2.12.0-alpha.1');
    }
  },

  _emberVersionRequiresModulesAPIPolyfill() {
    // once a version of Ember ships with the
    // emberjs/rfcs#176 modules natively this will
    // be updated to detect that and return false
    return true;
  },

  _getEmberModulesAPIBlacklist() {
    const blacklist = {
      '@ember/debug': ['assert', 'deprecate', 'warn'],
    };

    if (this._shouldBlacklistEmberString()) {
      blacklist['@ember/string'] = [
        'fmt', 'loc', 'w',
        'decamelize', 'dasherize', 'camelize',
        'classify', 'underscore', 'capitalize',
        'setStrings', 'getStrings', 'getString'
      ];
    }

    if (this._shouldBlacklistJQuery()) {
      blacklist['jquery'] = ['default'];
    }

    return blacklist;
  },

  _isProjectName(dependency) {
    return this.project.name && this.project.name() === dependency;
  },

  _isTransitiveDependency(dependency) {
    return (
      !(dependency in this.parent.dependencies()) &&
      !(dependency in this.project.dependencies())
    )
  },

  _shouldBlacklistEmberString() {
    let packageName = '@ember/string';
    if (this._isProjectName(packageName)) { return true; }
    if (this._isTransitiveDependency(packageName)) { return false; }

    let checker = new VersionChecker(this.parent).for(packageName, 'npm');

    return checker.exists();
  },

  _shouldBlacklistJQuery() {
    let packageName = '@ember/jquery';
    if (this._isProjectName(packageName)) { return true; }
    if (this._isTransitiveDependency(packageName)) { return false; }

    let checker = new VersionChecker(this.parent).for(packageName, 'npm');

    return checker.gte('0.6.0');
  },

  // detect if running babel would do nothing... and do nothing instead
  _shouldDoNothing(options) {
    return !options.sourceMaps && !options.plugins.length;
  }
};
