'use strict';

const path = require('path');
const DependencyVersionChecker = require('./dependency-version-checker');
const getProject = require('./get-project');

class BowerDependencyVersionChecker extends DependencyVersionChecker {
  constructor(parent, name) {
    super(parent, name);

    let project = getProject(this._parent._addon);

    let bowerDependencyPath = path.join(
      project.root,
      project.bowerDirectory,
      this.name
    );

    this._jsonPath = path.join(bowerDependencyPath, '.bower.json');
    this._fallbackJsonPath = path.join(bowerDependencyPath, 'bower.json');
    this._type = 'bower';
  }
}

module.exports = BowerDependencyVersionChecker;
