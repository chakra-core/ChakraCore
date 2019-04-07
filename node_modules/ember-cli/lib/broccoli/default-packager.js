'use strict';

const p = require('ember-cli-preprocess-registry/preprocessors');
const path = require('path');
const concat = require('broccoli-concat');
const Funnel = require('broccoli-funnel');
const mergeTrees = require('./merge-trees');
const ConfigLoader = require('broccoli-config-loader');
const UnwatchedDir = require('broccoli-source').UnwatchedDir;
const ConfigReplace = require('broccoli-config-replace');
const emberAppUtils = require('../utilities/ember-app-utils');
const funnelReducer = require('broccoli-funnel-reducer');
const addonProcessTree = require('../utilities/addon-process-tree');

const preprocessCss = p.preprocessCss;
const preprocessJs = p.preprocessJs;
const preprocessTemplates = p.preprocessTemplates;
const preprocessMinifyCss = p.preprocessMinifyCss;

const DEFAULT_BOWER_PATH = 'bower_components';
const DEFAULT_VENDOR_PATH = 'vendor';
const EMBER_CLI_INTERNAL_FILES_PATH = '/vendor/ember-cli/';
const EMBER_CLI_FILES = [
  'app-boot.js',
  'app-config.js',
  'app-prefix.js',
  'app-suffix.js',
  'test-support-prefix.js',
  'test-support-suffix.js',
  'tests-prefix.js',
  'tests-suffix.js',
  'vendor-prefix.js',
  'vendor-suffix.js',
];

const configReplacePatterns = emberAppUtils.configReplacePatterns;

function callAddonsPreprocessTreeHook(project, type, tree) {
  return addonProcessTree(project, 'preprocessTree', type, tree);
}

function callAddonsPostprocessTreeHook(project, type, tree) {
  return addonProcessTree(project, 'postprocessTree', type, tree);
}

/*
  Creates an object with lists of files to be concatenated into `vendor.js` file.

  Given a map that looks like:

  ```
  {
    'assets/vendor.js': [
      'vendor/ember-cli-shims/app-shims.js',
      'vendor/loader/loader.js',
      'vendor/ember-resolver/legacy-shims.js',
      ...
    ]
  }
  ```

  Produces an object that looks like:

  ```
  {
    headerFiles: [
     'vendor/ember-cli/vendor-prefix.js',
     'vendor/loader/loader.js',
     'vendor/ember/jquery/jquery.js',
     'vendor/ember/ember.debug.js',
     'vendor/ember-cli-shims/app-shims.js',
     'vendor/ember-resolver/legacy-shims.js'
    ],
    inputFiles: [
      'addon-tree-output/**\/*.js'
    ],
    footerFiles: [
      'vendor/ember-cli/vendor-suffix.js'
    ],
    annotation: 'Vendor JS'
  }
  ```

  @private
  @method getVendorFiles
  @param {Object} files A list of files to include into `<file-name>.js`
  @param {Boolean} isMainVendorFile Boolean flag to indicate if we are dealing with `vendor.js` file
  @return {Object} An object with lists of files to be concatenated into `vendor.js` file.
 */
function getVendorFiles(files, isMainVendorFile) {
  return {
    headerFiles: files,
    inputFiles: isMainVendorFile ? ['addon-tree-output/**/*.js'] : [],
    footerFiles: isMainVendorFile ? ['vendor/ember-cli/vendor-suffix.js'] : [],
  };
}

/**
 * Responsible for packaging Ember.js application.
 *
 * @class DefaultPackager
 * @constructor
 */
module.exports = class DefaultPackager {
  constructor(options) {
    this._cachedTests = null;
    this._cachedBower = null;
    this._cachedVendor = null;
    this._cachedPublic = null;
    this._cachedConfig = null;
    this._cachedJavascript = null;
    this._cachedProcessedSrc = null;
    this._cachedProcessedIndex = null;
    this._cachedTransformedTree = null;
    this._cachedProcessedStyles = null;
    this._cachedProcessedTemplates = null;
    this._cachedProcessedJavascript = null;
    this._cachedEmberCliInternalTree = null;
    this._cachedProcessedAdditionalAssets = null;
    this._cachedProcessedAppAndDependencies = null;

    this.options = options || {};

    this.env = this.options.env;
    this.name = this.options.name;
    this.autoRun = this.options.autoRun;
    this.project = this.options.project;
    this.registry = this.options.registry;
    this.sourcemaps = this.options.sourcemaps;
    this.minifyCSS = this.options.minifyCSS;
    this.distPaths = this.options.distPaths;
    this.areTestsEnabled = this.options.areTestsEnabled;
    this.styleOutputFiles = this.options.styleOutputFiles;
    this.scriptOutputFiles = this.options.scriptOutputFiles;
    this.storeConfigInMeta = this.options.storeConfigInMeta;
    this.customTransformsMap = this.options.customTransformsMap;
    this.additionalAssetPaths = this.options.additionalAssetPaths;
    this.vendorTestStaticStyles = this.options.vendorTestStaticStyles;
    this.legacyTestFilesToAppend = this.options.legacyTestFilesToAppend;
    this.isModuleUnificationEnabled = this.options.isModuleUnificationEnabled;
  }

  /*
   * Replaces variables in `index.html` file with values from
   * `config/environment.js` and returns a single tree that contains
   * `index.html` file with populated values.
   *
   * Input tree:
   *
   * ```
   * /
   * ├── addon-tree-output/
   * ├── the-best-app-ever/
   * ├── bower_components/
   * └── vendor/
   * ```
   *
   * Returns:
   *
   * ```
   * /
   * └── index.html
   * ```
   *
   * @private
   * @method processIndex
   * @param {BroccoliTree}
   * @return {BroccoliTree}
  */
  processIndex(tree) {
    if (this._cachedProcessedIndex === null) {
      let indexFilePath = this.distPaths.appHtmlFile;

      let index = new Funnel(tree, {
        allowEmtpy: true,
        include: [`${this.name}/index.html`],
        getDestinationPath: () => indexFilePath,
        annotation: 'Classic: index.html',
      });

      if (this.isModuleUnificationEnabled) {
        let srcIndex = new Funnel(tree, {
          files: ['src/ui/index.html'],
          getDestinationPath: () => indexFilePath,
          annotation: 'Module Unification `index.html`',
        });

        index = mergeTrees([
          index,
          srcIndex,
        ], {
          overwrite: true,
          annotation: 'Classic And Module Unification `index.html`',
        });
      }

      let patterns = configReplacePatterns({
        addons: this.project.addons,
        autoRun: this.autoRun,
        storeConfigInMeta: this.storeConfigInMeta,
        isModuleUnification: this.isModuleUnificationEnabled,
      });

      this._cachedProcessedIndex = new ConfigReplace(index, this.packageConfig(), {
        configPath: path.join(this.name, 'config', 'environments', `${this.env}.json`),
        files: [indexFilePath],
        patterns,
      });
    }

    return this._cachedProcessedIndex;
  }

  /*
   * Combines compiled javascript, external files (node modules, bower
   * components), vendor files and processed configuration (based on the
   * environment) into a single tree.
   *
   * Input tree:
   *
   * ```
   * /
   * ├── addon-tree-output/
   * ├── the-best-app-ever/
   * ├── bower_components/
   * └── vendor/
   * ```
   *
   * Changes are made "inline" so the output tree has the same structure.
   *
   * @private
   * @method processAppAndDependencies
   * @param {BroccoliTree}
   * @return {BroccoliTree}
  */
  processAppAndDependencies(allTrees) {
    if (this._cachedProcessedAppAndDependencies === null) {
      let config = this.packageConfig();
      let internal = this.packageEmberCliInternalFiles();
      let templates = this.processTemplates(allTrees);

      let trees = [
        allTrees,
        templates,
        this.processSrc(allTrees),
      ].filter(Boolean);

      let mergedTree = mergeTrees(trees, {
        annotation: 'TreeMerger (preprocessedApp & templates)',
        overwrite: true,
      });

      let external = this.applyCustomTransforms(allTrees);
      let postprocessedApp = this.processJavascript(mergedTree);

      let sourceTrees = [
        external,
        postprocessedApp,
        config,
        internal,
      ];

      this._cachedProcessedAppAndDependencies = mergeTrees(sourceTrees, {
        overwrite: true,
        annotation: 'Processed Application and Dependencies',
      });
    }

    return this._cachedProcessedAppAndDependencies;
  }

  /*
   * Adds additional assets to the results tree, given the following list:
   *
   * ```
   * [{
   *   src: 'vendor/font-awesome/fonts',
   *   file: 'FontAwesome.otf',
   *   dest: 'fonts'
   * }]
   * ```
   *
   * where `src` is a source path, `file` is a file name, and `dest` is a new
   * destination.
   *
   * @private
   * @method importAdditionalAssets
   * @param {BroccoliTree}
   * @return {BroccoliTree}
  */
  importAdditionalAssets(tree) {
    if (this._cachedProcessedAdditionalAssets === null) {
      let otherAssetTrees = funnelReducer(this.additionalAssetPaths).map(options => {
        let files = options.include.join(',');
        options.annotation = `${options.srcDir}/{${files}} => ${options.destDir}/{${files}}`;

        return new Funnel(tree, options);
      });

      this._cachedProcessedAdditionalAssets = mergeTrees(otherAssetTrees, {
        annotation: 'Processed Additional Assets',
      });
    }

    return this._cachedProcessedAdditionalAssets;
  }

  /*
   * Runs all registered transformations on the passed in tree and returns the
   * result.
   *
   * Passed-in tree:
   *
   * ```
   * /
   * ├── addon-tree-output/
   * ├── bower_components/
   * └── vendor/
   * ```
   *
   * `customTransformsMap` has information about files that needed to be
   * transformed and the actual transformation functions that are executed.
   *
   * @private
   * @method applyCustomTransforms
   * @param {BroccoliTree} External (vendor) tree
   * @return {BroccoliTree}
  */
  applyCustomTransforms(externalTree) {
    if (this._cachedTransformedTree === null) {
      this._cachedTransformedTree = externalTree;

      for (let customTransformEntry of this.customTransformsMap) {
        let transformName = customTransformEntry[0];
        let transformConfig = customTransformEntry[1];

        let transformTree = new Funnel(this._cachedTransformedTree, {
          files: transformConfig.files,
          annotation: `Funnel (custom transform: ${transformName})`,
        });

        this._cachedTransformedTree = mergeTrees([this._cachedTransformedTree, transformConfig.callback(transformTree, transformConfig.options)], {
          annotation: `TreeMerger (custom transform: ${transformName})`,
          overwrite: true,
        });
      }
    }

    return this._cachedTransformedTree;
  }

  /*
   * Returns a single tree with `ember-cli` internal files with the following
   * structure:
   *
   * ```
   * vendor/
   * └── ember-cli
   *     ├── app-boot.js
   *     ├── app-config.js
   *     ├── app-prefix.js
   *     ├── app-suffix.js
   *     ├── test-support-suffix.js
   *     ├── test-support-prefix.js
   *     ├── tests-prefix.js
   *     ├── tests-suffix.js
   *     ├── vendor-prefix.js
   *     └── vendor-suffix.js
   * ```
   *
   * Note, that the contents of these files is being matched against several
   * internal `ember-cli` variables, such as:
   *
   * + `{{MODULE_PREFIX}}`
   * + different types of `{{content-for}}` (`{{content-for 'app-boot'}}`)
   *
   * @private
   * @method packageEmberCliInternalFiles
   * @return {BroccoliTree}
  */
  packageEmberCliInternalFiles() {
    if (this._cachedEmberCliInternalTree === null) {
      let patterns = configReplacePatterns({
        addons: this.project.addons,
        autoRun: this.autoRun,
        storeConfigInMeta: this.storeConfigInMeta,
        isModuleUnification: this.isModuleUnificationEnabled,
      });

      let configTree = this.packageConfig();
      let configPath = path.join(this.name, 'config', 'environments', `${this.env}.json`);

      let emberCLITree = new ConfigReplace(new UnwatchedDir(__dirname), configTree, {
        configPath,
        files: EMBER_CLI_FILES,
        patterns,
      });

      this._cachedEmberCliInternalTree = new Funnel(emberCLITree, {
        files: EMBER_CLI_FILES,
        destDir: EMBER_CLI_INTERNAL_FILES_PATH,
        annotation: 'Packaged Ember CLI Internal Files',
      });
    }

    return this._cachedEmberCliInternalTree;
  }

  /*
   * Runs pre/post-processors hooks on the template files and returns a single
   * tree with the processed templates.
   *
   * Given a tree:
   *
   * ```
   * /
   * ├── addon-tree-output/
   * ├── bower_components/
   * ├── the-best-app-ever/
   * └── vendor/
   * ```
   *
   * Returns:
   *
   * ```
   * the-best-app-ever/
   * └── templates
   *     ├── application.js
   *     ├── error.js
   *     ├── index.js
   *     └── loading.js
   * ```
   *
   * @private
   * @method processTemplates
   * @param {BroccoliTree} tree
   * @return {BroccoliTree}
  */
  processTemplates(templates) {
    if (this._cachedProcessedTemplates === null) {
      let include = this.registry.extensionsForType('template')
        .map(extension => `**/*/template.${extension}`);

      let pods = new Funnel(templates, {
        include,
        exclude: ['templates/**/*'],
        annotation: 'Pod Templates',
      });
      let classic = new Funnel(templates, {
        srcDir: `${this.name}/templates`,
        destDir: `${this.name}/templates`,
        annotation: 'Classic Templates',
      });
      let mergedTemplates = mergeTrees([pods, classic], {
        overwrite: true,
        annotation: 'Pod & Classic Templates',
      });
      let preprocessedTemplatesFromAddons = callAddonsPreprocessTreeHook(
        this.project,
        'template',
        mergedTemplates
      );

      this._cachedProcessedTemplates = callAddonsPostprocessTreeHook(
        this.project,
        'template',
        preprocessTemplates(preprocessedTemplatesFromAddons, {
          registry: this.registry,
        })
      );
    }

    return this._cachedProcessedTemplates;
  }

  /*
   * Runs pre/post-processors hooks on the javascript files and returns a single
   * tree with the processed javascript.
   *
   * Given a tree:
   *
   * ```
   * /
   * ├── addon-tree-output/
   * ├── bower_components/
   * ├── the-best-app-ever/
   * └── vendor/
   * ```
   *
   * Returns:
   *
   * ```
   * the-best-app-ever/
   * ├── adapters
   * │   └── application.js
   * ├── app.js
   * ├── components
   * ├── controllers
   * ├── helpers
   * │   ├── and.js
   * │   ├── app-version.js
   * │   ├── await.js
   * │   ├── camelize.js
   * │   ├── cancel-all.js
   * │   ├── dasherize.js
   * │   ├── dec.js
   * │   ├── drop.js
   * │   └── eq.js
   * ...
   * ```
   *
   * @private
   * @method processJavascript
   * @param {BroccoliTree} tree
   * @return {BroccoliTree}
  */
  processJavascript(tree) {
    if (this._cachedProcessedJavascript === null) {
      let javascript = new Funnel(tree, {
        srcDir: this.name,
        destDir: this.name,
        annotation: '',
      });
      let app = callAddonsPreprocessTreeHook(this.project, 'js', javascript);

      let preprocessedApp = preprocessJs(app, '/', this.name, {
        registry: this.registry,
      });

      this._cachedProcessedJavascript = callAddonsPostprocessTreeHook(
        this.project,
        'js',
        preprocessedApp
      );
    }

    return this._cachedProcessedJavascript;
  }

  /*
   * Compiles application css files, runs pre/post-processors hooks on the them,
   * concatenates them into one application and vendor files and returns a
   * single tree.
   *
   * Given an input tree that looks like:
   *
   * ```
   * addon-tree-output/
   *   ...
   * bower_components/
   *   hint.css/
   *   ...
   * the-best-app-ever/
   *   styles/
   *   ...
   * vendor/
   *   font-awesome/
   *   ...
   * ```
   *
   * Returns:
   *
   * ```
   * assets/
   *   the-best-app-ever.css
   *   vendor.css
   * ```
   *
   * @private
   * @method packageStyles
   * @return {BroccoliTree}
  */
  packageStyles(tree) {
    if (this._cachedProcessedStyles === null) {
      let cssMinificationEnabled = this.minifyCSS.enabled;
      let options = {
        outputPaths: this.distPaths.appCssFile,
        registry: this.registry,
        minifyCSS: this.minifyCSS.options,
      };

      let stylesAndVendor, preprocessedStyles;

      if (this._cachedProcessedSrc !== null && this.isModuleUnificationEnabled === true) {
        stylesAndVendor = mergeTrees([
          this._cachedProcessedSrc,
          tree,
        ], {
          annotation: 'Module Unification Styles',
        });
        preprocessedStyles = new Funnel(this._cachedProcessedSrc, {
          srcDir: `${this.name}/assets`,
          destDir: 'assets',
        });
      } else {
        stylesAndVendor = callAddonsPreprocessTreeHook(this.project, 'css', tree);

        preprocessedStyles = preprocessCss(
          stylesAndVendor,
          '/app/styles',
          '/assets',
          options
        );
      }

      let vendorStyles = [];
      for (let outputFile in this.styleOutputFiles) {
        let isMainVendorFile = outputFile === this.distPaths.vendorCssFile;
        let headerFiles = this.styleOutputFiles[outputFile];
        let inputFiles = isMainVendorFile ? ['addon-tree-output/**/*.css'] : [];

        vendorStyles.push(concat(stylesAndVendor, {
          headerFiles,
          inputFiles,
          outputFile,
          allowNone: true,
          annotation: `Concat: Vendor Styles${outputFile}`,
        }));
      }

      vendorStyles = mergeTrees(vendorStyles, {
        annotation: 'TreeMerger (vendorStyles)',
        overwrite: true,
      });

      if (cssMinificationEnabled === true) {
        options.minifyCSS.registry = options.registry;
        preprocessedStyles = preprocessMinifyCss(preprocessedStyles, options.minifyCSS);
        vendorStyles = preprocessMinifyCss(vendorStyles, options.minifyCSS);
      }

      this._cachedProcessedStyles = callAddonsPostprocessTreeHook(this.project, 'css', mergeTrees([
        preprocessedStyles,
        vendorStyles,
      ], {
        annotation: 'Packaged Styles',
      }));
    }

    return this._cachedProcessedStyles;
  }

  /*
   * Runs pre/post-processors hooks on the application files (javascript,
   * styles, templates) and returns a single tree with the processed ones.
   *
   * Used only when `MODULE_UNIFICATION` experiment is enabled.
   *
   * Given a tree:
   *
   * ```
   * /
   * └── src
   *     ├── main.js
   *     ├── resolver.js
   *     ├── router.js
   *     └── ui
   *         ├── components
   *         ├── index.html
   *         ├── routes
   *         │   └── application
   *         │   └── template.hbs
   *         └── styles
   *             └── app.css
   * ```
   *
   * Returns the tree with the same structure, but with processed files.
   *
   * @private
   * @method processSrc
   * @param {BroccoliTree} tree
   * @return {BroccoliTree}
  */
  processSrc(tree) {
    if (this._cachedProcessedSrc === null && this.isModuleUnificationEnabled) {
      let src = new Funnel(tree, {
        srcDir: 'src',
        destDir: 'src',
        exclude: ['**/*-test.js'],
        annotation: 'Module Unification Src',
      });

      let srcAfterPreprocessTreeHook = callAddonsPreprocessTreeHook(this.project, 'src', src);

      let options = {
        outputPaths: this.distPaths.appCssFile,
        registry: this.registry,
      };

      // TODO: This isn't quite correct (but it does function properly in most cases),
      // and should be re-evaluated before enabling the `MODULE_UNIFICATION` feature
      let srcAfterStylePreprocessing = preprocessCss(srcAfterPreprocessTreeHook, '/src/ui/styles', '/assets', options);

      let srcAfterTemplatePreprocessing = preprocessTemplates(srcAfterPreprocessTreeHook, {
        registry: this.registry,
        annotation: 'Process Templates: src',
      });

      let srcAfterPostprocessTreeHook = callAddonsPostprocessTreeHook(
        this.project,
        'src',
        srcAfterTemplatePreprocessing
      );

      this._cachedProcessedSrc = new Funnel(mergeTrees([
        srcAfterPostprocessTreeHook,
        srcAfterStylePreprocessing,
      ], { ovewrite: true }), {
        srcDir: '/',
        destDir: this.name,
        annotation: 'Funnel: src',
      });
    }

    return this._cachedProcessedSrc;
  }

  /*
   * Given an input tree, returns a properly assembled Broccoli tree with bower
   * components.
   *
   * Given a tree:
   *
   * ```
   * ├── ember.js/
   * ├── pusher/
   * └── raven-js/
   * ```
   *
   * Returns:
   *
   * ```
   * [bowerDirectory]/
   * ├── ember.js/
   * ├── pusher/
   * └── raven-js/
   * ```
   *
   * @private
   * @method packageBower
   * @param {BroccoliTree} tree
   * @param {String} bowerDirectory Custom path to bower components
  */
  packageBower(tree, bowerDirectory) {
    if (this._cachedBower === null) {
      this._cachedBower = new Funnel(tree, {
        destDir: bowerDirectory || DEFAULT_BOWER_PATH,
        annotation: 'Packaged Bower',
      });
    }

    return this._cachedBower;
  }

  /*
   * Given an input tree, returns a properly assembled Broccoli tree with vendor
   * files.
   *
   * Given a tree:
   *
   * ```
   * ├── babel-polyfill/
   * ├── ember-cli-shims/
   * ├── ember-load-initializers/
   * ├── ember-qunit/
   * ├── ember-resolver/
   * ├── sinon/
   * └── tether/
   * ```
   *
   * Returns:
   *
   * ```
   * vendor/
   * ├── babel-polyfill/
   * ├── ember-cli-shims/
   * ├── ember-load-initializers/
   * ├── ember-qunit/
   * ├── ember-resolver/
   * ├── sinon/
   * └── tether/
   * ```
   *
   * @private
   * @method packageVendor
   * @param {BroccoliTree} tree
  */
  packageVendor(tree) {
    if (this._cachedVendor === null) {
      this._cachedVendor = new Funnel(tree, {
        destDir: DEFAULT_VENDOR_PATH,
        annotation: 'Packaged Vendor',
      });
    }

    return this._cachedVendor;
  }

  /*
   * Given an input tree, returns a properly assembled Broccoli tree with tests
   * files.
   *
   * Given a tree:
   *
   * ```
   * addon-tree-output/
   * bower_components/
   * the-best-app-ever/
   * tests/
   * ├── acceptance/
   * ├── helpers/
   * ├── index.html
   * ├── integration/
   * ├── test-helper.js
   * └── unit/
   * ```
   *
   * Returns:
   *
   * ```
   * [name]/
   * └── tests
   *     ├── acceptance/
   *     ├── helpers/
   *     ├── index.html
   *     ├── integration/
   *     ├── test-helper.js
   *     └── unit/
   * ```
   *
   * @private
   * @method processTests
   * @param {BroccoliTree} tree
  */
  processTests(tree) {
    if (this._cachedTests === null) {
      let addonTestSupportTree = new Funnel(tree, {
        srcDir: 'tests/addon-test-support',
        destDir: 'addon-test-support',
      });

      let testTree = new Funnel(tree, {
        srcDir: 'tests',
        exclude: ['addon-test-support/**/*'],
      });

      if (this.isModuleUnificationEnabled) {

        let testSrcTree = new Funnel(tree, {
          srcDir: 'src',
          include: ['**/*-test.js'],
          annotation: 'Module Unification Tests',
        });
        testTree = mergeTrees([testTree, testSrcTree], {
          annotation: 'Merge MU Tests',
        });

      }

      let treeToCompile = new Funnel(testTree, {
        destDir: `${this.name}/tests`,
        annotation: 'Tests To Process',
      });

      treeToCompile = callAddonsPreprocessTreeHook(this.project, 'test', treeToCompile);

      let preprocessedTests = preprocessJs(treeToCompile, '/tests', this.name, {
        registry: this.registry,
      });

      let mergedTestTrees = mergeTrees([addonTestSupportTree, preprocessedTests], {
        overwrite: true,
        annotation: 'Packaged Tests',
      });

      this._cachedTests = callAddonsPostprocessTreeHook(this.project, 'test', mergedTestTrees);
    }

    return this._cachedTests;
  }

  /*
   * Concatenates all test files into one, as follows:
   *
   * Given an input tree that looks like:
   *
   * ```
   * addon-tree-output/
   * bower_components/
   * the-best-app-ever/
   * tests/
   * ├── acceptance/
   * ├── helpers/
   * ├── index.html
   * ├── integration/
   * ├── test-helper.js
   * └── unit/
   * vendor/
   * ```
   *
   * Returns:
   *
   * ```
   * assets/
   *   tests.js
   *   test.map (if sourcemaps are enabled)
   *   test-support.js
   *   test-support.map (if sourcemaps are enabled)
   *   test-support.css
   * ```
   *
   * @private
   * @method packageTests
   * @param {BroccoliTree}
   * @return {BroccoliTree}
  */
  packageTests(tree) {
    let coreTestTree = this.processTests(tree);

    let testIndex = this.processTestIndex(tree);
    let appTestTree = this.packageApplicationTests(coreTestTree);
    let testFilesTree = this.packageTestFiles(tree, coreTestTree);

    return mergeTrees([testIndex, appTestTree, testFilesTree], {
      annotation: 'Packaged Tests',
    });
  }

  /*
   * Replaces variables in `tests/index.html` file with values from
   * `config/environment.js` and returns a single tree that contains
   * `index.html` file with populated values.
   *
   * Input tree:
   *
   * ```
   * /
   * ├── addon-tree-output/
   * ├── the-best-app-ever/
   * ├── bower_components/
   * ├── tests/
   * └── vendor/
   * ```
   *
   * Returns:
   *
   * ```
   * └── tests
   *     └── index.html/
   * ```
   *
   * @private
   * @method processTestIndex
   * @param {BroccoliTree}
   * @return {BroccoliTree}
  */
  processTestIndex(tree) {
    let index = new Funnel(tree, {
      srcDir: '/tests',
      files: ['index.html'],
      destDir: '/tests',
      annotation: 'Funnel (test index)',
    });

    let patterns = configReplacePatterns({
      addons: this.project.addons,
      autoRun: this.autoRun,
      storeConfigInMeta: this.storeConfigInMeta,
      isModuleUnification: this.isModuleUnificationEnabled,
    });

    let configPath = path.join(this.name, 'config', 'environments', 'test.json');

    return new ConfigReplace(index, this.packageConfig(), {
      configPath,
      files: ['tests/index.html'],
      env: 'test',
      patterns,
    });
  }

  /*
   * Wraps application configuration into AMD module:
   *
   * ```javascript
   * define('the-best-app-ever/config/environment', [], function() {
   *   // read the meta tag that contains escaped configuration from
   *   // `index.html` and return as an object
   * });
   * ```
   *
   * Given a tree:
   *
   * ```
   * environments/
   * ├── development.json
   * └── test.json
   * ```
   *
   * Returns:
   *
   * ```
   * └── vendor
   *     └── ember-cli
   *         └── app-config.js
   * ```
   * @private
   * @method packageTestApplicationConfig
  */
  packageTestApplicationConfig() {
    let files = ['app-config.js'];
    let patterns = configReplacePatterns({
      addons: this.project.addons,
      autoRun: this.autoRun,
      storeConfigInMeta: this.storeConfigInMeta,
      isModuleUnification: this.isModuleUnificationEnabled,
    });

    let configPath = path.join(this.name, 'config', 'environments', `test.json`);
    let emberCLITree = new ConfigReplace(new UnwatchedDir(__dirname), this.packageConfig(), {
      configPath,
      files,
      patterns,
    });

    return new Funnel(emberCLITree, {
      files,
      srcDir: '/',
      destDir: '/vendor/ember-cli/',
      annotation: 'Funnel (test-app-config-tree)',
    });
  }

  /*
   * Concatenates all application test files into one, as follows:
   *
   * Given an input tree that looks like:
   *
   * ```
   * addon-tree-output/
   * bower_components/
   * the-best-app-ever/
   * tests/
   * ├── acceptance/
   * ├── helpers/
   * ├── index.html
   * ├── integration/
   * ├── test-helper.js
   * └── unit/
   * vendor/
   * ```
   *
   * Returns:
   *
   * ```
   * assets/
   *   tests.js
   *   test.map (if sourcemaps are enabled)
   * ```
   *
   * @private
   * @method packageApplicationTests
   * @param {BroccoliTree}
   * @return {BroccoliTree}
  */
  packageApplicationTests(tree) {
    let appTestTrees = [].concat(
      this.packageEmberCliInternalFiles(),
      this.packageTestApplicationConfig(),
      tree
    ).filter(Boolean);

    appTestTrees = mergeTrees(appTestTrees, {
      overwrite: true,
      annotation: 'TreeMerger (appTestTrees)',
    });

    return concat(appTestTrees, {
      inputFiles: [`${this.name}/tests/**/*.js`],
      headerFiles: ['vendor/ember-cli/tests-prefix.js'],
      footerFiles: ['vendor/ember-cli/app-config.js', 'vendor/ember-cli/tests-suffix.js'],
      outputFile: this.distPaths.testJsFile,
      annotation: 'Concat: App Tests',
      sourceMapConfig: this.sourcemaps,
    });
  }

  /*
   * Concatenates all test support files into one, as follows:
   *
   * Given an input tree that looks like:
   *
   * ```
   * addon-tree-output/
   * bower_components/
   * the-best-app-ever/
   * tests/
   * ├── acceptance/
   * ├── helpers/
   * ├── index.html
   * ├── integration/
   * ├── test-helper.js
   * └── unit/
   * vendor/
   * ```
   *
   * Returns:
   *
   * ```
   * assets/
   *   test-support.js
   *   test-support.map (if sourcemaps are enabled)
   *   test-support.css
   * ```
   *
   * @private
   * @method packageTestFiles
   * @return {BroccoliTree}
  */
  packageTestFiles(tree, coreTestTree) {
    let testSupportPath = this.distPaths.testSupportJsFile;

    testSupportPath = testSupportPath.testSupport || testSupportPath;

    let emberCLITree = this.packageEmberCliInternalFiles();

    let headerFiles = [].concat(
      'vendor/ember-cli/test-support-prefix.js',
      this.legacyTestFilesToAppend
    );

    let inputFiles = ['addon-test-support/**/*.js'];

    let footerFiles = ['vendor/ember-cli/test-support-suffix.js'];

    let external = this.applyCustomTransforms(tree);

    let baseMergedTree = mergeTrees([emberCLITree, tree, external, coreTestTree], {
      overwrite: true,
    });
    let testJs = concat(baseMergedTree, {
      headerFiles,
      inputFiles,
      footerFiles,
      outputFile: testSupportPath,
      annotation: 'Concat: Test Support JS',
      allowNone: true,
      sourceMapConfig: this.sourcemaps,
    });

    let testemPath = path.join(__dirname, 'testem');
    testemPath = path.dirname(testemPath);

    let testemTree = new Funnel(new UnwatchedDir(testemPath), {
      files: ['testem.js'],
      annotation: 'Funnel (testem)',
    });

    let sourceTrees = [
      testemTree,
      testJs,
    ];

    if (this.vendorTestStaticStyles.length > 0) {
      sourceTrees.push(
        concat(tree, {
          headerFiles: this.vendorTestStaticStyles,
          outputFile: this.distPaths.testSupportCssFile,
          annotation: 'Concat: Test Support CSS',
          sourceMapConfig: this.sourcemaps,
        })
      );
    }

    return mergeTrees(sourceTrees, {
      overwrite: true,
      annotation: 'TreeMerger (testFiles)',
    });
  }

  /*
   * Returns a flattened input tree.
   *
   * Given a tree:
   *
   * ```
   * public
   * ├── crossdomain.xml
   * ├── ember-fetch
   * ├── favicon.ico
   * ├── images
   * └── robots.txt
   * ```
   *
   * Returns:
   *
   * ```
   * ├── crossdomain.xml
   * ├── ember-fetch
   * ├── favicon.ico
   * ├── images
   * └── robots.txt
   * ```
   *
   * @private
   * @method packagePublic
   * @param {BroccoliTree} tree
   * @return {BroccoliTree}
  */
  packagePublic(tree) {
    if (this._cachedPublic === null) {
      this._cachedPublic = new Funnel(tree, {
        srcDir: 'public',
        destDir: '.',
      });
    }

    return this._cachedPublic;
  }

  /*
   * Given an input tree, returns a properly assembled Broccoli tree with
   * configuration files.
   *
   * Given a tree:
   *
   * ```
   * environments/
   * ├── development.json
   * └── test.json
   * ```
   *
   * Returns:
   *
   * ```
   * └── [name]
   *     └── config
   *         └── environments
   *             ├── development.json
   *             └── test.json
   * ```
   * @private
   * @method packageConfig
  */
  packageConfig() {
    let env = this.env;
    let name = this.name;
    let project = this.project;
    let configPath = this.project.configPath();

    if (this._cachedConfig === null) {
      let configTree = new ConfigLoader(path.dirname(configPath), {
        env,
        tests: this.areTestsEnabled || false,
        project,
      });

      this._cachedConfig = new Funnel(configTree, {
        destDir: `${name}/config`,
        annotation: 'Packaged Config',
      });
    }

    return this._cachedConfig;
  }

  /*
   * Concatenates all javascript Broccoli trees into one, as follows:
   *
   * Given an input tree that looks like:
   *
   * ```
   * addon-tree-output/
   *   ember-ajax/
   *   ember-data/
   *   ember-engines/
   *   ember-resolver/
   *   ...
   * bower_components/
   *   usertiming/
   *   sinonjs/
   *   ...
   * the-best-app-ever/
   *   components/
   *   config/
   *   helpers/
   *   routes/
   *   ...
   * vendor/
   *   ...
   *   babel-core/
   *   ...
   *   broccoli-concat/
   *   ...
   *   ember-cli-template-lint/
   *   ...
   * ```
   *
   * Returns:
   *
   * ```
   * assets/
   *   the-best-app-ever.js
   *   the-best-app-ever.map (if sourcemaps are enabled)
   *   vendor.js
   *   vendor.map (if sourcemaps are enabled)
   * ```
   *
   * @private
   * @method packageJavascript
   * @return {BroccoliTree}
  */
  packageJavascript(tree) {
    if (this._cachedJavascript === null) {
      let applicationJs = this.processAppAndDependencies(tree);

      let vendorFilePath = this.distPaths.vendorJsFile;
      this.scriptOutputFiles[vendorFilePath].unshift('vendor/ember-cli/vendor-prefix.js');

      let appJs = this.packageApplicationJs(applicationJs);
      let vendorJs = this.packageVendorJs(applicationJs);

      this._cachedJavascript = mergeTrees([appJs, vendorJs], {
        overwrite: true,
        annotation: 'Packaged Javascript',
      });
    }

    return this._cachedJavascript;
  }

  /*
   * Concatenates all application's javascript Broccoli trees into one, as follows:
   *
   * Given an input tree that looks like:
   *
   * ```
   * addon-tree-output/
   *   ember-ajax/
   *   ember-data/
   *   ember-engines/
   *   ember-resolver/
   *   ...
   * bower_components/
   *   usertiming/
   *   sinonjs/
   *   ...
   * the-best-app-ever/
   *   components/
   *   config/
   *   helpers/
   *   routes/
   *   ...
   * vendor/
   *   ...
   *   babel-core/
   *   ...
   *   broccoli-concat/
   *   ...
   *   ember-cli-template-lint/
   *   ...
   * ```
   *
   * Returns:
   *
   * ```
   * assets/
   *   the-best-app-ever.js
   *   the-best-app-ever.map (if sourcemaps are enabled)
   * ```
   *
   * @private
   * @method packageApplicationJs
   * @return {BroccoliTree}
  */
  packageApplicationJs(tree) {
    let inputFiles = [`${this.name}/**/*.js`];
    let headerFiles = [
      'vendor/ember-cli/app-prefix.js',
    ];
    let footerFiles = [
      'vendor/ember-cli/app-suffix.js',
      'vendor/ember-cli/app-config.js',
      'vendor/ember-cli/app-boot.js',
    ];

    return concat(tree, {
      inputFiles,
      headerFiles,
      footerFiles,
      outputFile: this.distPaths.appJsFile,
      annotation: 'Packaged Application Javascript',
      separator: '\n;',
      sourceMapConfig: this.sourcemaps,
    });
  }

  /*
   * Concatenates all application's vendor javascript Broccoli trees into one, as follows:
   *
   * Given an input tree that looks like:
   * ```
   * addon-tree-output/
   *   ember-ajax/
   *   ember-data/
   *   ember-engines/
   *   ember-resolver/
   *   ...
   * bower_components/
   *   usertiming/
   *   sinonjs/
   *   ...
   * the-best-app-ever/
   *   components/
   *   config/
   *   helpers/
   *   routes/
   *   ...
   * vendor/
   *   ...
   *   babel-core/
   *   ...
   *   broccoli-concat/
   *   ...
   *   ember-cli-template-lint/
   *   ...
   * ```
   *
   * Returns:
   *
   * ```
   * assets/
   *   vendor.js
   *   vendor.map (if sourcemaps are enabled)
   * ```
   *
   * @method packageVendorJs
   * @param {BroccoliTree} tree
   * @return {BroccoliTree}
  */
  packageVendorJs(tree) {
    let importPaths = Object.keys(this.scriptOutputFiles);

    // iterate over the keys and concat files
    // to support scenarios like
    // app.import('vendor/foobar.js', { outputFile: 'assets/baz.js' });
    let vendorTrees = importPaths.map(importPath => {
      let files = this.scriptOutputFiles[importPath];
      let isMainVendorFile = importPath === this.distPaths.vendorJsFile;

      const vendorObject = getVendorFiles(files, isMainVendorFile);

      return concat(tree, {
        inputFiles: vendorObject.inputFiles,
        headerFiles: vendorObject.headerFiles,
        footerFiles: vendorObject.footerFiles,
        outputFile: importPath,
        annotation: `Package ${importPath}`,
        separator: '\n;',
        sourceMapConfig: this.sourcemaps,
      });
    });

    return mergeTrees(vendorTrees, {
      overwrite: true,
      annotation: 'Packaged Vendor Javascript',
    });
  }
};
