'use strict';

const Blueprint = require('../../lib/models/blueprint');
const isPackageMissing = require('ember-cli-is-package-missing');

module.exports = {
  description: 'Generates a mock api endpoint in /api prefix.',

  anonymousOptions: [
    'endpoint-path',
  ],

  locals(options) {
    return {
      path: `/${options.entity.name.replace(/^\//, '')}`,
    };
  },

  beforeInstall(options) {
    let serverBlueprint = Blueprint.lookup('server', {
      ui: this.ui,
      analytics: this.analytics,
      project: this.project,
    });

    return serverBlueprint.install(options);
  },

  afterInstall(options) {
    if (!options.dryRun && isPackageMissing(this, 'express')) {
      return this.addPackagesToProject([
        { name: 'express', target: '^4.8.5' },
      ]);
    }
  },
};
