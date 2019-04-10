'use strict';

const stringUtil = require('ember-cli-string-utils');
const getURLFor = require('ember-source-channel-url');

let emberCanaryVersion;

module.exports = {
  description: 'Generates an Ember application with a module unification layout.',

  filesToRemove: [],

  locals(options) {
    let entity = options.entity;
    let rawName = entity.name;
    let name = stringUtil.dasherize(rawName);
    let namespace = stringUtil.classify(rawName);

    return {
      name,
      modulePrefix: name,
      namespace,
      emberCLIVersion: require('../../package').version,
      emberCanaryVersion,
      yarn: options.yarn,
      welcome: options.welcome,
    };
  },

  beforeInstall() {
    return getURLFor('canary').then(url => {
      emberCanaryVersion = url;
    });
  },

  fileMapTokens(options) {
    return {
      __component__() { return options.locals.component; },
    };
  },
};
