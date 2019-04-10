'use strict';

const Promise = require('rsvp').Promise;
const Blueprint = require('../models/blueprint');
const Task = require('../models/task');
const parseOptions = require('../utilities/parse-options');
const merge = require('ember-cli-lodash-subset').merge;

class GenerateTask extends Task {

  constructor(options) {
    super(options);
    this.blueprintFunction = 'install';
  }

  run(options) {
    let name = options.args[0];
    let isNotModuleUnification = !this.project.isModuleUnification();
    let noAddonBlueprint = ['mixin', 'blueprint-test'];

    let mainBlueprint = this.lookupBlueprint(name, options.ignoreMissingMain);
    let testBlueprint = this.lookupBlueprint(`${name}-test`, true);

    // lookup custom addon blueprint
    let addonBlueprint;
    if (isNotModuleUnification) {
      addonBlueprint = this.lookupBlueprint(`${name}-addon`, true);

      // otherwise, use default addon-import
      if (noAddonBlueprint.indexOf(name) < 0 && !addonBlueprint && options.args[1]) {
        let mainBlueprintSupportsAddon = mainBlueprint && mainBlueprint.supportsAddon() && isNotModuleUnification;

        if (mainBlueprintSupportsAddon) {
          addonBlueprint = this.lookupBlueprint('addon-import', true);
        }
      }
    }

    if (options.ignoreMissingMain && !mainBlueprint) {
      return Promise.resolve();
    }

    if (options.dummy) {
      // don't install test or addon reexport for dummy
      if (this.project.isEmberCLIAddon()) {
        testBlueprint = null;
        addonBlueprint = null;
      }
    }

    let entity = {
      name: options.args[1],
      options: parseOptions(options.args.slice(2)),
    };

    let blueprintOptions = {
      target: this.project.root,
      entity,
      ui: this.ui,
      analytics: this.analytics,
      project: this.project,
      settings: this.settings,
      testing: this.testing,
      taskOptions: options,
      originBlueprintName: name,
    };

    blueprintOptions = merge(blueprintOptions, options || {});

    return mainBlueprint[this.blueprintFunction](blueprintOptions)
      .then(() => {
        if (!testBlueprint) { return; }

        if (testBlueprint.locals === Blueprint.prototype.locals) {
          testBlueprint.locals = function(options) {
            return mainBlueprint.locals(options);
          };
        }

        let testBlueprintOptions = merge({}, blueprintOptions, { installingTest: true });

        return testBlueprint[this.blueprintFunction](testBlueprintOptions);
      })
      .then(() => {
        if (!addonBlueprint || name.match(/-addon/)) { return; }
        if (!this.project.isEmberCLIAddon() && blueprintOptions.inRepoAddon === null) { return; }

        if (addonBlueprint.locals === Blueprint.prototype.locals) {
          addonBlueprint.locals = function(options) {
            return mainBlueprint.locals(options);
          };
        }

        let addonBlueprintOptions = merge({}, blueprintOptions, { installingAddon: true });

        return addonBlueprint[this.blueprintFunction](addonBlueprintOptions);
      });
  }

  lookupBlueprint(name, ignoreMissing) {
    return Blueprint.lookup(name, {
      paths: this.project.blueprintLookupPaths(),
      ignoreMissing,
    });
  }
}

module.exports = GenerateTask;
