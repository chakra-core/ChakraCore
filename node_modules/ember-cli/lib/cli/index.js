'use strict';

const willInterruptProcess = require('../utilities/will-interrupt-process');
const instrumentation = require('../utilities/instrumentation');
const getConfig = require('../utilities/get-config');
const ciInfo = require('ci-info');

let initInstrumentation;
if (instrumentation.instrumentationEnabled()) {
  const heimdall = require('heimdalljs');
  let initInstrumentationToken = heimdall.start('init');
  initInstrumentation = {
    token: initInstrumentationToken,
    node: heimdall.current,
  };
}

// Main entry point
const requireAsHash = require('../utilities/require-as-hash');
const packageConfig = require('../../package.json');
const logger = require('heimdalljs-logger')('ember-cli:cli/index');
const merge = require('ember-cli-lodash-subset').merge;
const path = require('path');
const heimdall = require('heimdalljs');

let version = packageConfig.version;
let name = packageConfig.name;
let trackingCode = packageConfig.trackingCode;

function loadCommands() {
  let token = heimdall.start('load-commands');
  const Command = require('../models/command');
  let hash = requireAsHash('../commands/*.js', Command);
  token.stop();
  return hash;
}

function loadTasks() {
  let token = heimdall.start('load-tasks');
  const Task = require('../models/task');
  let hash = requireAsHash('../tasks/*.js', Task);
  token.stop();
  return hash;
}

function clientId() {
  const ConfigStore = require('configstore');
  let configStore = new ConfigStore('ember-cli');
  let id = configStore.get('client-id');

  if (id) {
    return id;
  } else {
    id = require('uuid').v4().toString();
    configStore.set('client-id', id);
    return id;
  }
}

function configureLogger(env) {
  let depth = Number(env['DEBUG_DEPTH']);
  if (depth) {
    let logConfig = require('heimdalljs').configFor('logging');
    logConfig.depth = depth;
  }
}

// Options: Array cliArgs, Stream inputStream, Stream outputStream, EventEmitter process
module.exports = function(options) {
  // `process` should be captured before we require any libraries which
  // may use `process.exit` work arounds for async cleanup.
  willInterruptProcess.capture(options.process || process);

  let UI = options.UI || require('console-ui');
  const CLI = require('./cli');
  let Leek = options.Leek || require('leek');
  const Project = require('../models/project');
  let config = getConfig(options.Yam);

  configureLogger(process.env);

  // TODO: one UI (lib/models/project.js also has one for now...)
  let ui = new UI({
    inputStream: options.inputStream,
    outputStream: options.outputStream,
    errorStream: options.errorStream || process.stderr,
    errorLog: options.errorLog || [],
    ci: ciInfo.isCI || (/^(dumb|emacs)$/).test(process.env.TERM),
    writeLevel: (process.argv.indexOf('--silent') !== -1) ? 'ERROR' : undefined,
  });

  let leekOptions;

  let disableAnalytics = (options.cliArgs &&
    (options.cliArgs.indexOf('--disable-analytics') > -1 ||
    options.cliArgs.indexOf('-v') > -1 ||
    options.cliArgs.indexOf('--version') > -1)) ||
    config.get('disableAnalytics');

  let defaultLeekOptions = {
    trackingCode,
    globalName: name,
    name: clientId(),
    version,
    silent: disableAnalytics,
  };

  let defaultUpdateCheckerOptions = {
    checkForUpdates: false,
  };

  if (config.get('leekOptions')) {
    leekOptions = merge(defaultLeekOptions, config.get('leekOptions'));
  } else {
    leekOptions = defaultLeekOptions;
  }

  logger.info('leek: %o', leekOptions);

  let leek = new Leek(leekOptions);

  let cli = new CLI({
    ui,
    analytics: leek,
    testing: options.testing,
    name: options.cli ? options.cli.name : 'ember',
    disableDependencyChecker: options.disableDependencyChecker,
    root: options.cli ? options.cli.root : path.resolve(__dirname, '..', '..'),
    npmPackage: options.cli ? options.cli.npmPackage : 'ember-cli',
    initInstrumentation,
  });

  let project = Project.projectOrnullProject(ui, cli);

  let environment = {
    tasks: loadTasks(),
    cliArgs: options.cliArgs,
    commands: loadCommands(),
    project,
    settings: merge(defaultUpdateCheckerOptions, config.getAll()),
  };

  return cli.run(environment).finally(() => willInterruptProcess.release());
};
