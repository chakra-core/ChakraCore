'use strict';

const Project = require('../../lib/models/project');
const Instrumentation = require('../../lib/models/instrumentation');
const MockUI = require('console-ui/mock');

// eslint-disable-next-line node/no-unpublished-require

class MockProject extends Project {
  constructor(options = {}) {
    let root = options.root || process.cwd();
    let pkg = options.pkg || {};
    let ui = options.ui || new MockUI();
    let instr = new Instrumentation({
      ui,
      initInstrumentation: {
        token: null,
        node: null,
      },
    });
    let cli = options.cli || {
      instrumentation: instr,
    };

    super(root, pkg, ui, cli);
  }

  require(file) {
    if (file === './server') {
      return function() {
        return {
          listen() { arguments[arguments.length - 1](); },
        };
      };
    }
  }

  config() {
    return this._config || {
      baseURL: '/',
      locationType: 'auto',
    };
  }

  has(key) {
    return (/server/.test(key));
  }

  name() {
    return 'mock-project';
  }

  hasDependencies() {
    return true;
  }

  dependencies() {
    return [];
  }

  isEmberCLIAddon() {
    return false;
  }

  isModuleUnification() {
    return false;
  }
}

module.exports = MockProject;
