'use strict';

/**
@module ember-cli
*/
const defaultsDeep = require('ember-cli-lodash-subset').defaultsDeep;
const Funnel = require('broccoli-funnel');
const fs = require('fs');

const EmberApp = require('./ember-app');

class EmberAddon extends EmberApp {
  /**
    EmberAddon is used during addon development.

    @class EmberAddon
    @extends EmberApp
    @constructor
    @param {Object} [defaults]
    @param {Object} [options={}] Configuration options
  */
  constructor(defaults, options) {
    if (arguments.length === 0) {
      options = {};
    } else if (arguments.length === 1) {
      options = defaults;
    } else {
      defaultsDeep(options, defaults);
    }

    process.env.EMBER_ADDON_ENV = process.env.EMBER_ADDON_ENV || 'development';
    let overrides = {
      name: 'dummy',
      configPath: './tests/dummy/config/environment',
      trees: {
        app: 'tests/dummy/app',
        public: 'tests/dummy/public',
        src: null,
        styles: 'tests/dummy/app/styles',
        templates: 'tests/dummy/app/templates',
        tests: new Funnel('tests', {
          exclude: [/^dummy/],
        }),
        vendor: null,
      },
      jshintrc: {
        tests: './tests',
        app: './tests/dummy',
      },
    };

    if (!fs.existsSync('tests/dummy/app')) {
      overrides.trees.app = null;
      overrides.trees.styles = null;
      overrides.trees.templates = null;
    }
    if (fs.existsSync('tests/dummy/src')) {
      overrides.trees.src = 'tests/dummy/src';
      overrides.trees.styles = 'tests/dummy/src/ui/styles';
    }

    if (fs.existsSync('tests/dummy/vendor')) {
      overrides.trees.vendor = 'tests/dummy/vendor';
    }

    super(defaultsDeep(options, overrides));
  }
}

module.exports = EmberAddon;
