'use strict';

const clone = require('ember-cli-lodash-subset').clone;
const merge = require('ember-cli-lodash-subset').merge;
const Command = require('../models/command');
const Promise = require('rsvp').Promise;
const SilentError = require('silent-error');
const validProjectName = require('../utilities/valid-project-name');
const normalizeBlueprint = require('../utilities/normalize-blueprint-option');
const mergeBlueprintOptions = require('../utilities/merge-blueprint-options');
const isYarnProject = require('../utilities/is-yarn-project');

module.exports = Command.extend({
  name: 'init',
  description: 'Creates a new ember-cli project in the current folder.',
  works: 'everywhere',

  availableOptions: [
    { name: 'dry-run',     type: Boolean, default: false, aliases: ['d'] },
    { name: 'verbose',     type: Boolean, default: false, aliases: ['v'] },
    { name: 'blueprint',   type: String,                  aliases: ['b'] },
    { name: 'skip-npm',    type: Boolean, default: false, aliases: ['sn'] },
    { name: 'skip-bower',  type: Boolean, default: false, aliases: ['sb'] },
    { name: 'welcome',     type: Boolean, default: true, description: 'Installs and uses {{ember-welcome-page}}. Use --no-welcome to skip it.' },
    { name: 'yarn',        type: Boolean }, // no default means use yarn if the blueprint has a yarn.lock
    { name: 'name',        type: String,  default: '',    aliases: ['n'] },
  ],

  anonymousOptions: [
    '<glob-pattern>',
  ],

  _defaultBlueprint() {
    if (this.project.isEmberCLIAddon()) {
      if (this.project.isModuleUnification()) {
        return 'module-unification-addon';
      }
      return 'addon';
    } else {
      if (this.project.isModuleUnification()) {
        return 'module-unification-app';
      }
      return 'app';
    }
  },

  beforeRun: mergeBlueprintOptions,

  run(commandOptions, rawArgs) {
    if (commandOptions.dryRun) {
      commandOptions.skipNpm = true;
      commandOptions.skipBower = true;
    }

    let project = this.project;
    let packageName = (commandOptions.name !== '.' && commandOptions.name) || project.name();

    if (!packageName) {
      let message = `The \`ember ${this.name}\` command requires a ` +
        `package.json in current folder with name attribute or a specified name via arguments. ` +
        `For more details, use \`ember help\`.`;

      return Promise.reject(new SilentError(message));
    }

    let blueprintOpts = clone(commandOptions);

    if (blueprintOpts.yarn === undefined) {
      blueprintOpts.yarn = isYarnProject(project.root);
    }

    merge(blueprintOpts, {
      rawName: packageName,
      targetFiles: rawArgs || '',
      rawArgs: rawArgs.toString(),
      blueprint: normalizeBlueprint(blueprintOpts.blueprint || this._defaultBlueprint()),
    });

    if (!validProjectName(packageName)) {
      return Promise.reject(new SilentError(`We currently do not support a name of \`${packageName}\`.`));
    }

    return this.runTask('InstallBlueprint', blueprintOpts)
      .then(() => {
        if (!commandOptions.skipNpm) {
          return this.runTask('NpmInstall', {
            verbose: commandOptions.verbose,
            useYarn: commandOptions.yarn,
          });
        }
      })
      .then(() => {
        if (!commandOptions.skipBower) {
          return this.runTask('BowerInstall', {
            verbose: commandOptions.verbose,
          });
        }
      })
      .then(() => {
        if (commandOptions.skipGit === false) {
          return this.runTask('GitInit', commandOptions, rawArgs);
        }
      });
  },
});
