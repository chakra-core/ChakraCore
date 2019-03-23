'use strict';

/* eslint-env node */

const BowerDependencyVersionChecker = require('./bower-dependency-version-checker');
const NPMDependencyVersionChecker = require('./npm-dependency-version-checker');
const getProject = require('./get-project');

class VersionChecker {
  constructor(addon) {
    this._addon = addon;
  }

  for(name, type) {
    if (type === 'bower') {
      return new BowerDependencyVersionChecker(this, name);
    } else {
      return new NPMDependencyVersionChecker(this, name);
    }
  }

  forEmber() {
    let project = getProject(this._addon);
    let checker = project === this._addon ? this : new VersionChecker(project);
    let emberVersionChecker = checker.for('ember-source', 'npm');

    if (emberVersionChecker.version) {
      return emberVersionChecker;
    }

    return checker.for('ember', 'bower');
  }
}

module.exports = VersionChecker;
