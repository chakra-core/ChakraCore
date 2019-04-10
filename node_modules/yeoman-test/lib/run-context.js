'use strict';
var crypto = require('crypto');
var path = require('path');
var os = require('os');
var assert = require('assert');
var _ = require('lodash');
var Promise = require('pinkie-promise');
var yeoman = require('yeoman-environment');
var util = require('util');
var rimraf = require('rimraf');
var EventEmitter = require('events').EventEmitter;
var helpers = require('./');
var TestAdapter = require('./adapter').TestAdapter;

/**
 * This class provide a run context object to façade the complexity involved in setting
 * up a generator for testing
 * @constructor
 * @param {String|Function} Generator - Namespace or generator constructor. If the later
 *                                       is provided, then namespace is assumed to be
 *                                       'gen:test' in all cases
 * @param {Object} [settings]
 * @param {Boolean} [settings.tmpdir=true] - Automatically run this generator in a tmp dir
 * @param {String} [settings.resolved] - File path to the generator (only used if Generator is a constructor)
 * @param {String} [settings.namespace='gen:test'] - Namespace (only used if Generator is a constructor)
 * @return {this}
 */

var RunContext = module.exports = function RunContext(Generator, settings) {
  this._asyncHolds = 0;
  this.ran = false;
  this.inDirSet = false;
  this.args = [];
  this.options = {};
  this.answers = {};
  this.localConfig = null;
  this.dependencies = [];
  this.Generator = Generator;
  this.settings = _.extend({ tmpdir: true, namespace: 'gen:test' }, settings);
  this.withOptions({
    force: true,
    skipCache: true,
    skipInstall: true
  });
  setTimeout(this._run.bind(this), 10);
};

util.inherits(RunContext, EventEmitter);

/**
 * Hold the execution until the returned callback is triggered
 * @return {Function} Callback to notify the normal execution can resume
 */

RunContext.prototype.async = function () {
  this._asyncHolds++;

  return function () {
    this._asyncHolds--;
    this._run();
  }.bind(this);
};

/**
 * Method called when the context is ready to run the generator
 * @private
 */

RunContext.prototype._run = function () {
  if (!this.inDirSet && this.settings.tmpdir) {
    this.inTmpDir();
  }

  if (this._asyncHolds !== 0 || this.ran) {
    return;
  }

  this.ran = true;
  var namespace;
  this.env = yeoman.createEnv([], {}, new TestAdapter());

  helpers.registerDependencies(this.env, this.dependencies);

  if (_.isString(this.Generator)) {
    namespace = this.env.namespace(this.Generator);
    this.env.register(this.Generator);
  } else {
    namespace = this.settings.namespace;
    this.env.registerStub(this.Generator, namespace, this.settings.resolved);
  }

  this.generator = this.env.create(namespace, {
    arguments: this.args,
    options: this.options
  });

  helpers.mockPrompt(this.generator, this.answers);

  if (this.localConfig) { // only mock local config when withLocalConfig was called
    helpers.mockLocalConfig(this.generator, this.localConfig);
  }

  this.generator.on('error', function (err) {
    if (!this.emit('error', err)) {
      throw err;
    }
  }.bind(this));
  this.generator.once('end', function () {
    helpers.restorePrompt(this.generator);
    this.emit('end');
    this.completed = true;
  }.bind(this));

  this.emit('ready', this.generator);
  this.generator.run();
};

/**
 * Return a promise representing the generator run process
 * @return {Promise} Promise resolved on end or rejected on error
 */
RunContext.prototype.toPromise = function () {
  return new Promise(function (resolve, reject) {
    this.on('end', function () {
      resolve(this.targetDirectory);
    }.bind(this));
    this.on('error', reject);
  }.bind(this));
};

/**
 * Promise `.then()` duck typing
 * @return {Promise}
 */
RunContext.prototype.then = function () {
  var promise = this.toPromise();
  return promise.then.apply(promise, arguments);
};


/**
 * Promise `.catch()` duck typing
 * @return {Promise}
 */
RunContext.prototype.catch = function () {
  var promise = this.toPromise();
  return promise.catch.apply(promise, arguments);
};

/**
 * Clean the provided directory, then change directory into it
 * @param  {String} dirPath - Directory path (relative to CWD). Prefer passing an absolute
 *                            file path for predictable results
 * @param {Function} [cb] - callback who'll receive the folder path as argument
 * @return {this} run context instance
 */

RunContext.prototype.inDir = function (dirPath, cb) {
  this.inDirSet = true;
  this.targetDirectory = dirPath;
  var release = this.async();
  var callBackThenRelease = _.flowRight(
    release,
    (cb || _.noop).bind(this, path.resolve(dirPath))
  );
  helpers.testDirectory(dirPath, callBackThenRelease);
  return this;
};

/**
 * Change directory without deleting directory content.
 * @param  {String} dirPath - Directory path (relative to CWD). Prefer passing an absolute
 *                            file path for predictable results
 * @return {this} run context instance
 */
RunContext.prototype.cd = function (dirPath) {
  this.inDirSet = true;
  this.targetDirectory = dirPath;
  dirPath = path.resolve(dirPath);
  try {
    process.chdir(dirPath);
  } catch (err) {
    throw new Error(err.message + ' ' + dirPath);
  }
  return this;
};

/**
 * Cleanup a temporary directy and change the CWD into it
 *
 * This method is called automatically when creating a RunContext. Only use it if you need
 * to use the callback.
 *
 * @param {Function} [cb] - callback who'll receive the folder path as argument
 * @return {this} run context instance
 */
RunContext.prototype.inTmpDir = function (cb) {
  var tmpdir = path.join(os.tmpdir(), crypto.randomBytes(20).toString('hex'));
  return this.inDir(tmpdir, cb);
};

/**
 * Clean the directory used for tests inside inDir/inTmpDir
 */
RunContext.prototype.cleanTestDirectory = function () {
  if (this.targetDirectory) {
    rimraf.sync(this.targetDirectory);
  }
};

/**
 * Provide arguments to the run context
 * @param  {String|Array} args - command line arguments as Array or space separated string
 * @return {this}
 */

RunContext.prototype.withArguments = function (args) {
  var argsArray = _.isString(args) ? args.split(' ') : args;
  assert(_.isArray(argsArray), 'args should be either a string separated by spaces or an array');
  this.args = this.args.concat(argsArray);
  return this;
};

/**
 * Provide options to the run context
 * @param  {Object} options - command line options (e.g. `--opt-one=foo`)
 * @return {this}
 */

RunContext.prototype.withOptions = function (options) {
  // Add options as both kebab and camel case. This is to stay backward compatibles with
  // the switch we made to meow for options parsing.
  Object.keys(options).forEach(function (key) {
    options[_.camelCase(key)] = options[key];
    options[_.kebabCase(key)] = options[key];
  });

  this.options = _.extend(this.options, options);
  return this;
};

/**
 * Mock the prompt with dummy answers
 * @param  {Object} answers - Answers to the prompt questions
 * @return {this}
 */

RunContext.prototype.withPrompts = function (answers) {
  this.answers = _.extend(this.answers, answers);
  return this;
};

/**
 * Provide dependent generators
 * @param {Array} dependencies - paths to the generators dependencies
 * @return {this}
 * @example
 * var angular = new RunContext('../../app');
 * angular.withGenerators([
 *   '../../common',
 *   '../../controller',
 *   '../../main',
 *   [helpers.createDummyGenerator(), 'testacular:app']
 * ]);
 * angular.on('end', function () {
 *   // assert something
 * });
 */

RunContext.prototype.withGenerators = function (dependencies) {
  assert(_.isArray(dependencies), 'dependencies should be an array');
  this.dependencies = this.dependencies.concat(dependencies);
  return this;
};

/**
 * Mock the local configuration with the provided config
 * @param  {Object} localConfig - should look just like if called config.getAll()
 * @return {this}
 */
RunContext.prototype.withLocalConfig = function (localConfig) {
  assert(_.isObject(localConfig), 'config should be an object');
  this.localConfig = localConfig;
  return this;
};
