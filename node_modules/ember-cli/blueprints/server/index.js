'use strict';

const isPackageMissing = require('ember-cli-is-package-missing');

module.exports = {
  description: 'Generates a server directory for mocks and proxies.',

  normalizeEntityName() {},

  afterInstall(options) {
    let isMorganMissing = isPackageMissing(this, 'morgan');
    let isGlobMissing = isPackageMissing(this, 'glob');

    let areDependenciesMissing = isMorganMissing || isGlobMissing;
    let libsToInstall = [];

    if (isMorganMissing) {
      libsToInstall.push({ name: 'morgan', target: '^1.3.2' });
    }

    if (isGlobMissing) {
      libsToInstall.push({ name: 'glob', target: '^4.0.5' });
    }

    if (!options.dryRun && areDependenciesMissing) {
      return this.addPackagesToProject(libsToInstall);
    }
  },

  files() {
    return ['server/index.js', this.hasJSHint() ? 'server/.jshintrc' : 'server/.eslintrc.js'];
  },

  hasJSHint() {
    if (this.project) {
      return 'ember-cli-jshint' in this.project.dependencies();
    }
  },
};
