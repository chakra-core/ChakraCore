'use strict';

const path = require('path');
const FixturifyProject = require('fixturify-project');
const Project = require('../../lib/models/project');
const MockCLI = require('./mock-cli');

// used in these tests to ensure we are only
// operating on the addons added here
class ProjectWithoutInternalAddons extends Project {
  supportedInternalAddonPaths() {
    return [];
  }
}

function prepareAddon(addon) {
  addon.pkg.keywords.push('ember-addon');
  addon.pkg['ember-addon'] = { };
  addon.files['index.js'] = 'module.exports = { name: require("./package").name };';
}

module.exports = class EmberCLIFixturifyProject extends FixturifyProject {
  writeSync() {
    super.writeSync(...arguments);
    this._hasWrriten = true;
  }

  buildProjectModel(ProjectClass = ProjectWithoutInternalAddons) {
    if (this._hasWrriten !== false) {
      this.writeSync();
    }

    let pkg = JSON.parse(this.toJSON('package.json'));
    let cli = new MockCLI();
    let root = path.join(this.root, this.name);

    return new ProjectClass(root, pkg, cli.ui, cli);
  }

  addAddon(name, version = '0.0.0', cb) {
    return this.addDependency(name, version, addon => {
      prepareAddon(addon);

      if (typeof cb === 'function') {
        cb(addon);
      }
    });
  }

  addDevAddon(name, version = '0.0.0', cb) {
    return this.addDevDependency(name, version, addon => {
      prepareAddon(addon);
      if (typeof cb === 'function') {
        cb(addon);
      }
    });
  }

  addInRepoAddon(name, version = '0.0.0', cb) {
    const inRepoAddon = new FixturifyProject(name, version, project => {
      project.pkg.keywords.push('ember-addon');
      project.pkg['ember-addon'] = { };
      project.files['index.js'] = 'module.exports = { name: require("./package").name };';

      if (typeof cb === 'function') {
        cb(project);
      }
    });

    // configure the current project to have an ember-addon configured at the appropriate path
    let addon = this.pkg['ember-addon'] = this.pkg['ember-addon'] || { };
    addon.paths = addon.paths || [];
    const addonPath = `lib/${name}`;

    if (addon.paths.find(path => path.toLowerCase() === addonPath.toLowerCase())) {
      throw new Error(`project: ${this.name} already contains the in-repo-addon: ${name}`);
    }

    addon.paths.push(addonPath);

    this.files.lib = this.files.lib || {};

    // insert inRepoAddon into files
    Object.assign(this.files.lib, inRepoAddon.toJSON());
  }
};
