'use strict';

const Command = require('../models/command');
const SilentError = require('silent-error');
const Promise = require('rsvp').Promise;

module.exports = Command.extend({
  name: 'install',
  description: 'Installs an ember-cli addon from npm.',
  aliases: ['i'],
  works: 'insideProject',

  availableOptions: [
    { name: 'save',       type: Boolean, default: false, aliases: ['S'] },
    { name: 'save-dev',   type: Boolean, default: true,  aliases: ['D'] },
    { name: 'save-exact', type: Boolean, default: false, aliases: ['E', 'exact'] },
    { name: 'yarn',       type: Boolean, description: 'Use --yarn to enforce yarn usage, or --no-yarn to enforce npm' }, // no default means use yarn if the blueprint has a yarn.lock
  ],

  anonymousOptions: [
    '<addon-name>',
  ],

  run(commandOptions, addonNames) {
    if (!addonNames.length) {
      let msg = 'The `install` command must take an argument with the name';
      msg += ' of an ember-cli addon. For installing all npm and bower ';
      msg += 'dependencies you can run `npm install && bower install`.';
      return Promise.reject(new SilentError(msg));
    }

    return this.runTask('AddonInstall', {
      'packages': addonNames,
      blueprintOptions: commandOptions,
    });
  },

  _env() {
    return {
      ui: this.ui,
      analytics: this.analytics,
      project: this.project,
      NpmInstallTask: this.tasks.NpmInstall,
      BlueprintTask: this.tasks.GenerateFromBlueprint,
    };
  },
});
