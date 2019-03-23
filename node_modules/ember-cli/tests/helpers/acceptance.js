'use strict';

const symlinkOrCopySync = require('symlink-or-copy').sync;
const path = require('path');
const fs = require('fs-extra');
const runCommand = require('./run-command');
const hasGlobalYarn = require('../helpers/has-global-yarn');

let root = path.resolve(__dirname, '..', '..');

const PackageCache = require('../../tests/helpers/package-cache');

const quickTemp = require('quick-temp');
let dirs = {};

let runCommandOptions = {
  // Note: We must override the default logOnFailure logging, because we are
  // not inside a test.
  log() {
    return; // no output for initial application build
  },
};

function handleResult(result) {
  if (result.output) { console.log(result.output.join('\n')); }
  if (result.errors) { console.log(result.errors.join('\n')); }
  throw result;
}

function applyCommand(command, name /*, ...flags*/) {
  let flags = [].slice.call(arguments, 2, arguments.length);
  let binaryPath = path.resolve(path.join(__dirname, '..', '..', 'bin', 'ember'));
  let args = [binaryPath, command, name, '--disable-analytics', '--watcher=node', '--skip-git', runCommandOptions];

  flags.forEach(function(flag) {
    args.splice(2, 0, flag);
  });

  return runCommand.apply(undefined, args);
}

/**
 * Use `createTestTargets` in the before hook to do the initial
 * setup of a project. This will ensure that we limit the amount of times
 * we go to the network to fetch dependencies.
 * @param  {String} projectName The name of the project. Can be an app or addon.
 * @param  {Object} options
 * @property {String} options.command The command you want to run
 * @return {Promise}  The result of the running the command
 */
function createTestTargets(projectName, options) {
  let outputDir = quickTemp.makeOrReuse(dirs, projectName);

  options = options || {};
  options.command = options.command || 'new';

  return applyCommand(options.command, projectName, '--skip-npm', '--skip-bower', `--directory=${outputDir}`)
    .catch(handleResult);
}

/**
 * Tears down the targeted project download directory
 */
function teardownTestTargets() {
  // Remove all tmp directories created in this run.
  let dirKeys = Object.keys(dirs);
  for (let i = 0; i < dirKeys.length; i++) {
    quickTemp.remove(dirs, dirKeys[i]);
  }
}

/**
 * Creates symbolic links from the dependency temp directories
 * to the project that is under test.
 * @param  {String} projectName The name of the project under test
 * @return {String} The path to the hydrated fixture.
 */
function linkDependencies(projectName) {
  let sourceFixture = dirs[projectName]; // original fixture for this acceptance test.
  let runFixture = quickTemp.makeOrRemake(dirs, `${projectName}-clone`);

  fs.copySync(sourceFixture, runFixture);

  let nodeManifest = fs.readFileSync(path.join(runFixture, 'package.json'));

  let packageCache = new PackageCache(root);
  let packager = hasGlobalYarn ? 'yarn' : 'npm';

  packageCache.create('node', packager, nodeManifest, [{ name: 'ember-cli', path: root }]);

  let nodeModulesPath = path.join(runFixture, 'node_modules');
  symlinkOrCopySync(path.join(packageCache.get('node'), 'node_modules'), nodeModulesPath);

  process.chdir(runFixture);

  return runFixture;
}

/**
 * Clean a test run.
 */
function cleanupRun(projectName) {
  process.chdir(root);
  quickTemp.remove(dirs, `${projectName}-clone`);
}

module.exports = {
  createTestTargets,
  linkDependencies,
  teardownTestTargets,
  cleanupRun,
};
