'use strict';

/*
 * A list of "helper" functions to automate tedious tasks of generating and
 * validating folder structures.
*/

const assert = require('assert');

const DEFAULT_ADDON_MODULES = {
  'ember-cli-foobar': '^1.0.0',
};
const DEFAULT_BOWER_COMPONENTS = {};

const DEFAULT_NODE_MODULES = {
  'broccoli-asset-rev': {
    'index.js': 'module.exports = function() { return { name: "broccoli-asset-rev" }; };',
    'package.json': JSON.stringify({
      name: 'broccoli-asset-rev',
      keywords: ['ember-addon'],
    }),
  },
  'ember-cli-htmlbars': {
    'index.js': `module.exports = {
        name: 'ember-cli-htmlbars',
        setupPreprocessorRegistry(type, registry) {
          registry.add('template', {
            name: 'ember-cli-htmlbars',
            ext: 'hbs',
            toTree(tree) { return tree; }
          });
        }
      };
      `,
    'package.json': JSON.stringify({
      name: 'ember-cli-htmlbars',
      version: '2.0.1',
      keywords: ['ember-addon'],
    }),
  },
  'ember-source': {
    'index.js': `module.exports = function() {
        return {
          name: 'ember-source',
          paths: {
            debug: 'vendor/ember/ember.debug.js',
            prod: 'vendor/ember/ember.prod.js',
            testing: 'vendor/ember/ember-testing.js',
            shims: undefined,
            jquery: 'vendor/ember/jquery/jquery.js',
          }
        };
      };
      `,
    'package.json': JSON.stringify({
      name: 'ember-source',
      version: '3.0.0-beta.1',
      keywords: ['ember-addon'],
      paths: {
        debug: 'vendor/ember/ember.debug.js',
        prod: 'vendor/ember/ember.prod.js',
        testing: 'vendor/ember/ember-testing.js',
        shims: undefined,
        jquery: 'vendor/ember/jquery/jquery.js',
      },
    }),
  },
};

const DEFAULT_VENDOR = {
  'loader': {
    'loader.js': 'W',
  },
  'ember': {
    'jquery': {
      'jquery.js': 'window.$ = function() {}',
    },
    'ember.js': 'window.Ember = {};',
    'ember.debug.js': 'window.Ember = {};',
  },
  'ember-cli': {
    'app-boot.js': '!!!',
    'app-config.js': 'SPARTA',
    'app-prefix.js': 'THIS',
    'app-suffix.js': 'IS',
    'test-support-prefix.js': 'If a clod be washed away by the sea,',
    'test-support-suffix.js': 'Europe is the less.',
    'tests-prefix.js': 'As well as if a promontory were.',
    'tests-suffix.js': 'As well as if a manor of thine own',
    'vendor-prefix.js': 'HELLO',
    'vendor-suffix.js': '!',
  },
  'ember-cli-shims': {
    'app-shims.js': 'L',
  },
  'ember-resolver': {
    'legacy-shims.js': 'D',
  },
};

const DEFAULT_SOURCE = {
  'index.html': '<html></html>',
  'router.js': 'export default class {}',
  'resolver.js': `export default class {}`,
  routes: {
    'index.js': 'export default class {}',
  },
  styles: {
    'app.css': 'html,body{height:100%}',
  },
  templates: {
    'application.hbs': '{{outlet}}',
  },
  config: {
    'environment.js': `module.exports = function() { return { modulePrefix: 'the-best-app-ever' }; };`,
  },
  'package.json': JSON.stringify({
    name: 'the-best-app-ever',
    'devDependencies': {
      'broccoli-asset-rev': '^2.4.5',
      'ember-ajax': '^3.0.0',
      'ember-cli': '~3.0.0-beta.1',
      'ember-cli-app-version': '^3.0.0',
      'ember-cli-babel': '^6.6.0',
      'ember-cli-dependency-checker': '^3.1.0',
      'ember-cli-eslint': '^4.2.1',
      'ember-cli-htmlbars': '^3.0.0',
      'ember-cli-htmlbars-inline-precompile': '^1.0.0',
      'ember-cli-inject-live-reload': '^1.4.1',
      'ember-cli-sass': '^7.1.3',
      'ember-cli-shims': '^1.2.0',
      'ember-cli-sri': '^2.1.0',
      'ember-cli-uglify': '^2.0.0',
      'ember-data': '~3.0.0-beta.1',
      'ember-export-application-global': '^2.0.0',
      'ember-load-initializers': '^1.0.0',
      'ember-qunit': '^3.4.1',
      'ember-resolver': '^4.0.0',
      'ember-source': '~3.0.0-beta.1',
      'loader.js': '^4.2.3',
    },
  }),
  tests: {
    'test-helper.js': '// test-helper.js',
    integration: {
      components: {
        'foo-bar-test.js': '// foo-bar-test.js',
      },
    },
  },
};

/*
 * Generates an object that represents an unpackaged Ember application tree,
 * including application source, addons, vendor, bower (empty) and node.js files.
 *
 * @param {String} name The name of the app
 * @param {Object} options Customize output object
 *
 * @return {Object}
*/
function getDefaultUnpackagedDist(name, options) {
  options = options || {};

  const addonModules = Object.assign({}, DEFAULT_ADDON_MODULES, options.addonModules);
  const bowerComponents = options.bowerComponents || DEFAULT_BOWER_COMPONENTS;
  const nodeModules = options.nodeModules || DEFAULT_NODE_MODULES;
  const application = Object.assign({}, DEFAULT_SOURCE, options.source);
  const vendor = Object.assign({}, DEFAULT_VENDOR, options.vendor);

  return {
    'addon-modules': addonModules,
    'bower_components': bowerComponents,
    'node_modules': nodeModules,
    [name]: application,
    vendor,
  };
}

/*
 * Validates that passed-in object has the following shape:
 *
 * ```javascript
 * {
 *   assets: {
 *     [name].js
 *     [name].map
 *     vendor.js
 *     vendor.map
 *   }
 * }
 * ```
 *
 * This shape corresponds to the "dist" folder structure that Ember CLI creates
 * after the build.
 *
 * If the shape does not correspond to the expected value, it throws an
 * exception.
*/
function validateDefaultPackagedDist(name, obj) {
  if (obj !== undefined || obj.assets !== undefined) {
    let result = Object.keys(obj.assets).sort();

    let valid = [
      `${name}.js`,
      `${name}.map`,
      'vendor.js',
      'vendor.map',
    ];

    assert.deepStrictEqual(result, valid, `Expected [${valid}] but got [${result}]`);
  } else {
    throw new Error('Validation Error: Packaged files must be nested under `assets` folder');
  }
}

/*
 * Generates an object that represents a "dependency" on the disk. Could be used
 * both for generating bower and node dependencies.
 *
 * ```javascript
 * getDependencyFor('moment', {
 *   'file1.js: 'content',
 *   'file2.js': 'content'
 * });
 * ```
*/
function getDependencyFor(key, value) {
  return {
    [key]: value,
  };
}

/*
 * Generates the object that represents an application's registry where all
 * file processors are stored.
 *
 * It takes two arguments: a type of files you want to register custom processor
 * for and a function that takes a Broccoli tree and must return a Broccoli tree
 * as well.
 *
 * @param {String} registryType i.e. 'template', 'js', 'css', 'src', 'all'
 * @param {Function} fn Transormation that is applied to the input tree
*/
function setupRegistryFor(registryType, fn) {
  return {
    extensionsForType(type) {
      if (type === registryType) {
        return ['hbs'];
      }

      return [];
    },

    load(type) {
      if (type === registryType) {
        return [{
          toTree() {
            return fn.apply(this, arguments);
          },
        }];
      }

      return [];
    },
  };
}

/*
 * Generates the object that represents an application's registry where all
 * file processors are stored.
 *
 * It takes one argument: an object with the mapping from file type to a
 * "process" function. For example:
 *
 * ```
 * {
 *   js: tree => tree
 * }
 * ```
 *
 * @param {Object} registryMap
 * @return {Object}
*/
function setupRegistry(registryMap) {
  return {
    load(type) {
      if (registryMap[type]) {
        return [{
          toTree(tree) {
            return registryMap[type](tree);
          },
        }];
      }

      return [];
    },
  };
}

function setupProject(rootPath) {
  const path = require('path');
  const Project = require('../../lib/models/project');
  const MockCLI = require('./mock-cli');

  const packageContents = require(path.join(rootPath, 'package.json'));
  let cli = new MockCLI();

  return new Project(rootPath, packageContents, cli.ui, cli);
}

module.exports = {
  setupProject,
  setupRegistry,
  setupRegistryFor,
  validateDefaultPackagedDist,
  getDefaultUnpackagedDist,
  getDependencyFor,
  DEFAULT_SOURCE,
  DEFAULT_VENDOR,
  DEFAULT_NODE_MODULES,
  DEFAULT_ADDON_MODULES,
};
