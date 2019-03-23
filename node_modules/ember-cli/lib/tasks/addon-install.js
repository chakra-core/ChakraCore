'use strict';

const Task = require('../models/task');
const SilentError = require('silent-error');
const merge = require('ember-cli-lodash-subset').merge;
const getPackageBaseName = require('../utilities/get-package-base-name');
const Promise = require('rsvp').Promise;

class AddonInstallTask extends Task {
  constructor(options) {
    super(options);

    this.NpmInstallTask = this.NpmInstallTask || require('./npm-install');
    this.BlueprintTask = this.BlueprintTask || require('./generate-from-blueprint');
  }

  run(options) {
    const chalk = require('chalk');
    let ui = this.ui;
    let packageNames = options.packages;
    let blueprintOptions = options.blueprintOptions || {};
    let commandOptions = blueprintOptions;

    let npmInstall = new this.NpmInstallTask({
      ui: this.ui,
      analytics: this.analytics,
      project: this.project,
    });

    let blueprintInstall = new this.BlueprintTask({
      ui: this.ui,
      analytics: this.analytics,
      project: this.project,
      testing: this.testing,
    });

    ui.startProgress(chalk.green('Installing addon package'), chalk.green('.'));

    return npmInstall.run({
      'packages': packageNames,
      'save': commandOptions.save,
      'save-dev': !commandOptions.save,
      'save-exact': commandOptions.saveExact,
      useYarn: commandOptions.yarn,
    }).then(() => this.project.reloadAddons())
      .then(() => this.installBlueprint(blueprintInstall, packageNames, blueprintOptions))
      .finally(() => ui.stopProgress())
      .then(() => ui.writeLine(chalk.green('Installed addon package.')));
  }

  installBlueprint(install, packageNames, blueprintOptions) {
    let blueprintName, taskOptions, addonInstall = this;

    return packageNames.reduce((promise, packageName) =>
      promise.then(() => {
        blueprintName = addonInstall.findDefaultBlueprintName(packageName);
        if (blueprintName) {
          taskOptions = merge({
            args: [blueprintName],
            ignoreMissingMain: true,
          }, blueprintOptions || {});

          return install.run(taskOptions);

        } else {
          addonInstall.ui.writeWarnLine(`Could not figure out blueprint name from: "${packageName}". ` +
            `Please install the addon blueprint via "ember generate <addon-name>" if necessary.`);
        }
      }), Promise.resolve());
  }

  findDefaultBlueprintName(givenName) {
    let packageName = getPackageBaseName(givenName);
    if (!packageName) {
      return null;
    }

    let addon = this.project.findAddonByName(packageName);
    if (!addon) {
      throw new SilentError(`Install failed. Could not find addon with name: ${givenName}`);
    }

    let emberAddon = addon.pkg['ember-addon'];

    if (emberAddon && emberAddon.defaultBlueprint) {
      return emberAddon.defaultBlueprint;
    }

    return packageName;
  }
}

module.exports = AddonInstallTask;
