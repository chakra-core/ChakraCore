'use strict';
const addonBlueprint = require('../addon');
const getURLFor = require('ember-source-channel-url');

module.exports = Object.assign({}, addonBlueprint, {
  description: 'Generates an Ember addon with a module unification layout.',
  appBlueprintName: 'module-unification-app',

  fileMap: Object.assign({}, addonBlueprint.fileMap, {
    '^src.*': 'tests/dummy/:path',
    '^addon-src/.gitkeep': 'src/.gitkeep',
  }),


  locals(options) {
    return getURLFor('canary').then(url => {
      let result = addonBlueprint.locals(options);
      result.emberCanaryVersion = url;
      return result;
    });
  },
});
