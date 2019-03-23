'use strict';

const lookupCommand = require('../cli/lookup-command');
const stringUtils = require('ember-cli-string-utils');
const RootCommand = require('./root-command');
const versionUtils = require('./version-utils');
let emberCLIVersion = versionUtils.emberCLIVersion;

class JsonGenerator {
  constructor(options) {
    options = options || {};

    this.ui = options.ui;
    this.project = options.project;
    this.commands = options.commands;
    this.tasks = options.tasks;
  }

  generate(commandOptions) {
    let rootCommand = new RootCommand({
      ui: this.ui,
      project: this.project,
      commands: this.commands,
      tasks: this.tasks,
    });

    let json = rootCommand.getJson(commandOptions);
    json.version = emberCLIVersion();
    json.commands = [];
    json.addons = [];

    Object.keys(this.commands).forEach(function(commandName) {
      this._addCommandHelpToJson(commandName, commandOptions, json);
    }, this);

    if (this.project.eachAddonCommand) {
      this.project.eachAddonCommand((addonName, commands) => {
        this.commands = commands;

        let addonJson = { name: addonName };
        addonJson.commands = [];
        json.addons.push(addonJson);

        Object.keys(this.commands).forEach(function(commandName) {
          this._addCommandHelpToJson(commandName, commandOptions, addonJson);
        }, this);
      });
    }

    return json;
  }

  _addCommandHelpToJson(commandName, options, json) {
    let command = this._lookupCommand(commandName);
    if (!command.skipHelp && !command.unknown) {
      json.commands.push(command.getJson(options));
    }
  }

  _lookupCommand(commandName) {
    let Command = this.commands[stringUtils.classify(commandName)] ||
      lookupCommand(this.commands, commandName);

    return new Command({
      ui: this.ui,
      project: this.project,
      commands: this.commands,
      tasks: this.tasks,
    });
  }
}

module.exports = JsonGenerator;
