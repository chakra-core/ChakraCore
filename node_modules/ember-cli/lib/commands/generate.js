'use strict';

const path = require('path');
const chalk = require('chalk');
const Command = require('../models/command');
const Promise = require('rsvp').Promise;
const Blueprint = require('../models/blueprint');
const mergeBlueprintOptions = require('../utilities/merge-blueprint-options');
const _ = require('ember-cli-lodash-subset');
const EOL = require('os').EOL;
const SilentError = require('silent-error');

module.exports = Command.extend({
  name: 'generate',
  description: 'Generates new code from blueprints.',
  aliases: ['g'],
  works: 'insideProject',

  availableOptions: [
    { name: 'dry-run',       type: Boolean, default: false, aliases: ['d'] },
    { name: 'verbose',       type: Boolean, default: false, aliases: ['v'] },
    { name: 'pod',           type: Boolean, default: false, aliases: ['p'] },
    { name: 'classic',       type: Boolean, default: false, aliases: ['c'] },
    { name: 'dummy',         type: Boolean, default: false, aliases: ['dum', 'id'] },
    { name: 'in-repo-addon', type: String,  default: null,  aliases: ['in-repo', 'ir'] },
    { name: 'in',            type: String,  default: null, description: 'Runs a blueprint against an in repo addon. ' +
                                                                        'A path is expected, relative to the root of the project.' },
  ],

  anonymousOptions: [
    '<blueprint>',
  ],

  beforeRun: mergeBlueprintOptions,

  run(commandOptions, rawArgs) {
    let blueprintName = rawArgs[0];

    if (!blueprintName) {
      return Promise.reject(new SilentError('The `ember generate` command requires a ' +
                                            'blueprint name to be specified. ' +
                                            'For more details, use `ember help`.'));
    }

    let taskArgs = {
      args: rawArgs,
    };

    if (this.settings && this.settings.usePods) {
      if (commandOptions.pod) {
        let warning = 'Using both .ember-cli usePods settings and --pod flag ';
        warning += 'together has been deprecated.';
        this.ui.writeDeprecateLine(warning);
      }

      if (!commandOptions.classic) {
        commandOptions.pod = !commandOptions.pod;
      }
    }

    if (commandOptions.in) {
      let relativePath = path.relative(this.project.root, commandOptions.in);
      commandOptions.in = path.resolve(relativePath);
    }

    let taskOptions = _.merge(taskArgs, commandOptions || {});

    if (this.project.initializeAddons) {
      this.project.initializeAddons();
    }

    return this.runTask('GenerateFromBlueprint', taskOptions);
  },

  printDetailedHelp(options) {
    this.ui.writeLine(this.getAllBlueprints(options));
  },

  addAdditionalJsonForHelp(json, options) {
    json.availableBlueprints = this.getAllBlueprints(options);
  },

  getAllBlueprints(options) {
    let lookupPaths = this.project.blueprintLookupPaths();
    let blueprintList = Blueprint.list({ paths: lookupPaths });

    let output = '';

    let singleBlueprintName;
    if (options.rawArgs) {
      singleBlueprintName = options.rawArgs[0];
    }

    if (!singleBlueprintName && !options.json) {
      output += `${EOL}  Available blueprints:${EOL}`;
    }

    let collectionsJson = [];

    blueprintList.forEach(function(collection) {
      let result = this.getPackageBlueprints(collection, options, singleBlueprintName);
      if (options.json) {
        let collectionJson = {};
        collectionJson[collection.source] = result;
        collectionsJson.push(collectionJson);
      } else {
        output += result;
      }
    }, this);

    if (singleBlueprintName && !output && !options.json) {
      output = chalk.yellow(`The '${singleBlueprintName}' blueprint does not exist in this project.`) + EOL;
    }

    if (options.json) {
      return collectionsJson;
    } else {
      return output;
    }
  },

  getPackageBlueprints(collection, options, singleBlueprintName) {
    let verbose = options.verbose;
    let blueprints = collection.blueprints;

    if (!verbose) {
      blueprints = _.reject(blueprints, 'overridden');
    }

    let output = '';

    if (blueprints.length && !singleBlueprintName && !options.json) {
      output += `    ${collection.source}:${EOL}`;
    }

    let blueprintsJson = [];

    blueprints.forEach(function(blueprint) {
      let singleMatch = singleBlueprintName === blueprint.name;
      if (singleMatch) {
        verbose = true;
      }
      if (!singleBlueprintName || singleMatch) {
        // this may add default keys for printing
        blueprint.availableOptions.forEach(this.normalizeOption);

        if (options.json) {
          blueprintsJson.push(blueprint.getJson(verbose));
        } else {
          output += blueprint.printBasicHelp(verbose) + EOL;
        }
      }
    }, this);

    if (options.json) {
      return blueprintsJson;
    } else {
      return output;
    }
  },
});
