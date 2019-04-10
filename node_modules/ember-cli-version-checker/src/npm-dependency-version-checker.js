'use strict';

const DependencyVersionChecker = require('./dependency-version-checker');
const getProject = require('./get-project');
const resolvePackage = require('resolve-package-path');

module.exports = class NPMDependencyVersionChecker extends DependencyVersionChecker {
  constructor(parent, name) {
    super(parent, name);

    let addon = this._parent._addon;
    let basedir = addon.root || getProject(addon).root;
    this._jsonPath = resolvePackage(this.name, basedir);
    this._type = 'npm';
  }
};
