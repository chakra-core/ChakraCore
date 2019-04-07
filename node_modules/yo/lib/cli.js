#!/usr/bin/env node
'use strict';
const fs = require('fs');
const path = require('path');
const chalk = require('chalk');
const updateNotifier = require('update-notifier');
const Insight = require('insight');
const yosay = require('yosay');
const stringLength = require('string-length');
const rootCheck = require('root-check');
const meow = require('meow');
const list = require('cli-list');
const Tabtab = require('tabtab');
const pkg = require('../package.json');
const Router = require('./router');

const gens = list(process.argv.slice(2));

// Override http networking to go through a proxy ifone is configured
require('global-tunnel-ng').initialize();

/* eslint new-cap: 0, no-extra-parens: 0 */
const tabtab = new Tabtab.Commands.default({
  name: 'yo',
  completer: 'yo-complete'
});

const cli = gens.map(gen => {
  const minicli = meow({help: false, pkg, argv: gen});
  const opts = minicli.flags;
  const args = minicli.input;

  // Add un-camelized options too, for legacy
  // TODO: Remove some time in the future when generators have upgraded
  for (const key of Object.keys(opts)) {
    const legacyKey = key.replace(/[A-Z]/g, m => `-${m.toLowerCase()}`);
    opts[legacyKey] = opts[key];
  }

  return {opts, args};
});

const firstCmd = cli[0] || {opts: {}, args: {}};
const cmd = firstCmd.args[0];

const insight = new Insight({
  trackingCode: 'UA-31537568-1',
  pkg
});

function updateCheck() {
  const notifier = updateNotifier({pkg});
  const message = [];

  if (notifier.update) {
    message.push('Update available: ' + chalk.green.bold(notifier.update.latest) + chalk.gray(' (current: ' + notifier.update.current + ')'));
    message.push('Run ' + chalk.magenta('npm install -g ' + pkg.name) + ' to update.');
    console.log(yosay(message.join(' '), {maxLength: stringLength(message[0])}));
  }
}

function pre() {
  // Debugging helper
  if (cmd === 'doctor') {
    require('yeoman-doctor')();
    return;
  }

  if (cmd === 'completion') {
    return tabtab.install();
  }

  // Easteregg
  if (cmd === 'yeoman' || cmd === 'yo') {
    console.log(require('yeoman-character'));
    return;
  }

  init();
}

function createGeneratorList(env) {
  const generators = Object.keys(env.getGeneratorsMeta()).reduce((namesByGenerator, generator) => {
    const parts = generator.split(':');
    const generatorName = parts.shift();

    // If first time we found this generator, prepare to save all its sub-generators
    if (!namesByGenerator[generatorName]) {
      namesByGenerator[generatorName] = [];
    }

    // If sub-generator (!== app), save it
    if (parts[0] !== 'app') {
      namesByGenerator[generatorName].push(parts.join(':'));
    }

    return namesByGenerator;
  }, {});

  if (Object.keys(generators).length === 0) {
    return '  Couldn\'t find any generators, did you install any? Troubleshoot issues by running\n\n  $ yo doctor';
  }

  return Object.keys(generators).map(generator => {
    const subGenerators = generators[generator].map(subGenerator => `    ${subGenerator}`).join('\n');
    return `  ${generator}\n${subGenerators}`;
  }).join('\n');
}

function init() {
  const env = require('yeoman-environment').createEnv();

  env.on('error', err => {
    console.error('Error', process.argv.slice(2).join(' '), '\n');
    console.error(firstCmd.opts.debug ? err.stack : err.message);
    process.exit(err.code || 1);
  });

  // Lookup for every namespaces, within the environments.paths and lookups
  env.lookup(() => {
    const generatorList = createGeneratorList(env);

    // List generators
    if (firstCmd.opts.generators) {
      console.log('Available Generators:\n\n' + generatorList);
      return;
    }

    // Start the interactive UI if no generator is passed
    if (!cmd) {
      if (firstCmd.opts.help) {
        const usageText = fs.readFileSync(path.join(__dirname, 'usage.txt'), 'utf8');
        console.log(`${usageText}\nAvailable Generators:\n\n${generatorList}`);
        return;
      }

      runYo(env);
      return;
    }

    // More detailed error message
    // If users type in generator name with prefix 'generator-'
    if (cmd.startsWith('generator-')) {
      const generatorName = cmd.replace('generator-', '');
      const generatorCommand = chalk.yellow('yo ' + generatorName);

      console.log(chalk.red('Installed generators don\'t need the "generator-" prefix.'));
      console.log(`In the future, run ${generatorCommand} instead!\n`);

      env.run(generatorName, firstCmd.opts);

      return;
    }

    // Note: at some point, nopt needs to know about the generator options, the
    // one that will be triggered by the below args. Maybe the nopt parsing
    // should be done internally, from the args.
    for (const gen of cli) {
      env.run(gen.args, gen.opts);
    }
  });
}

function runYo(env) {
  const router = new Router(env, insight);
  router.insight.track('yoyo', 'init');
  router.registerRoute('help', require('./routes/help'));
  router.registerRoute('update', require('./routes/update'));
  router.registerRoute('run', require('./routes/run'));
  router.registerRoute('install', require('./routes/install'));
  router.registerRoute('exit', require('./routes/exit'));
  router.registerRoute('clearConfig', require('./routes/clear-config'));
  router.registerRoute('home', require('./routes/home'));

  process.once('exit', router.navigate.bind(router, 'exit'));

  router.updateAvailableGenerators();
  router.navigate('home');
}

rootCheck('\n' + chalk.red('Easy with the `sudo`. Yeoman is the master around here.') + '\n\nSince yo is a user command, there is no need to execute it with root\npermissions. If you\'re having permission errors when using yo without sudo,\nplease spend a few minutes learning more about how your system should work\nand make any necessary repairs.\n\nA quick solution would be to change where npm stores global packages by\nputting ~/npm/bin in your PATH and running:\n' + chalk.blue('npm config set prefix ~/npm') + '\n\nSee: https://github.com/sindresorhus/guides/blob/master/npm-global-without-sudo.md');

const insightMsg = chalk.gray('==========================================================================') +
chalk.yellow('\nWe\'re constantly looking for ways to make ') + chalk.bold.red(pkg.name) +
chalk.yellow(
  ' better! \nMay we anonymously report usage statistics to improve the tool over time? \n' +
  'More info: https://github.com/yeoman/insight & http://yeoman.io'
) +
chalk.gray('\n==========================================================================');

if (firstCmd.opts.insight === false) {
  insight.config.set('optOut', true);
} else if (firstCmd.opts.insight) {
  insight.config.set('optOut', false);
}

if (firstCmd.opts.insight !== false && insight.optOut === undefined) {
  insight.optOut = insight.config.get('optOut');
  insight.track('downloaded');
  insight.askPermission(insightMsg, pre);
} else {
  if (firstCmd.opts.insight !== false) {
    // Only track the two first subcommands
    insight.track(...firstCmd.args.slice(0, 2));
  }

  updateCheck();
  pre();
}
