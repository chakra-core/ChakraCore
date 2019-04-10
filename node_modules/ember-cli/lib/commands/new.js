'use strict';

const fs = require('fs-extra');
const chalk = require('chalk');
const Command = require('../models/command');
const RSVP = require('rsvp');
const Project = require('../models/project');
const SilentError = require('silent-error');
const validProjectName = require('../utilities/valid-project-name');
const normalizeBlueprint = require('../utilities/normalize-blueprint-option');
const mergeBlueprintOptions = require('../utilities/merge-blueprint-options');
const { isExperimentEnabled } = require('../experiments');

const rmdir = RSVP.denodeify(fs.remove);
const Promise = RSVP.Promise;

module.exports = Command.extend({
  name: 'new',
  description: `Creates a new directory and runs ${chalk.green('ember init')} in it.`,
  works: 'outsideProject',

  availableOptions: [
    { name: 'dry-run',     type: Boolean, default: false, aliases: ['d'] },
    { name: 'verbose',     type: Boolean, default: false, aliases: ['v'] },
    { name: 'blueprint',   type: String,  default: 'app', aliases: ['b'] },
    { name: 'skip-npm',    type: Boolean, default: false, aliases: ['sn'] },
    { name: 'skip-bower',  type: Boolean, default: false, aliases: ['sb'] },
    { name: 'skip-git',    type: Boolean, default: false, aliases: ['sg'] },
    { name: 'welcome',     type: Boolean, default: true, description: 'Installs and uses {{ember-welcome-page}}. Use --no-welcome to skip it.' },
    { name: 'yarn',        type: Boolean }, // no default means use yarn if the blueprint has a yarn.lock
    { name: 'directory',   type: String,                  aliases: ['dir'] },
  ],

  anonymousOptions: [
    '<app-name>',
  ],

  beforeRun: mergeBlueprintOptions,

  run(commandOptions, rawArgs) {
    if (isExperimentEnabled('MODULE_UNIFICATION')) {
      if (commandOptions.blueprint === 'app') {
        commandOptions.blueprint = 'module-unification-app';
      } else if (commandOptions.blueprint === 'addon') {
        commandOptions.blueprint = 'module-unification-addon';
      }
    }

    let packageName = rawArgs[0],
        message;

    commandOptions.name = rawArgs.shift();

    if (!packageName) {
      message = `The \`ember ${this.name}\` command requires a name to be specified. For more details, use \`ember help\`.`;

      return Promise.reject(new SilentError(message));
    }

    if (commandOptions.dryRun) {
      commandOptions.skipGit = true;
    }

    if (packageName === '.') {
      let blueprintName = commandOptions.blueprint === 'app' ? 'application' : commandOptions.blueprint;
      message = `Trying to generate an ${blueprintName} structure in this directory? Use \`ember init\` instead.`;

      return Promise.reject(new SilentError(message));
    }

    if (!validProjectName(packageName)) {
      message = `We currently do not support a name of \`${packageName}\`.`;

      return Promise.reject(new SilentError(message));
    }

    commandOptions.blueprint = normalizeBlueprint(commandOptions.blueprint);

    if (!commandOptions.directory) {
      commandOptions.directory = packageName;
    }

    let InitCommand = this.commands.Init;

    let initCommand = new InitCommand({
      ui: this.ui,
      analytics: this.analytics,
      tasks: this.tasks,
      project: Project.nullProject(this.ui, this.cli),
    });

    return this
      .runTask('CreateAndStepIntoDirectory', {
        directoryName: commandOptions.directory,
        dryRun: commandOptions.dryRun,
      })
      .then(opts => {
        initCommand.project.root = process.cwd();

        return initCommand
          .run(commandOptions, rawArgs)
          .catch(err => {
            let dirName = commandOptions.directory;
            process.chdir(opts.initialDirectory);
            return rmdir(dirName).then(() => {
              console.log(chalk.red(`Error creating new application. Removing generated directory \`./${dirName}\``));
              throw err;
            });
          });
      });
  },
});
