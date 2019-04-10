'use strict';
const onProcessInterrupt = require('../utilities/will-interrupt-process');
const fs = require('fs-extra');
const path = require('path');
const Promise = require('rsvp').Promise;
const CoreObject = require('core-object');
const SilentError = require('silent-error');
const chalk = require('chalk');
const attemptNeverIndex = require('../utilities/attempt-never-index');
const findBuildFile = require('../utilities/find-build-file');
const _resetTreeCache = require('./addon')._resetTreeCache;
const Sync = require('tree-sync');
const heimdall = require('heimdalljs');
const { isExperimentEnabled } = require('../experiments');

/**
 * Wrapper for the Broccoli [Builder](https://github.com/broccolijs/broccoli/blob/master/lib/builder.js) class.
 *
 * @private
 * @module ember-cli
 * @class Builder
 * @constructor
 * @extends Task
 */
class Builder extends CoreObject {

  constructor(options) {
    super(options);

    // Use Broccoli 2.0 by default, if this fails due to .read/.rebuild API, fallback to broccoli-builder
    this.broccoliBuilderFallback = false;
    this.setupBroccoliBuilder();

    this._instantiationStack = (new Error()).stack.replace(/[^\n]*\n/, '');
    this._cleanup = this.cleanup.bind(this);

    this._cleanupPromise = null;
    this._onProcessInterrupt = options.onProcessInterrupt || onProcessInterrupt;

    this._onProcessInterrupt.addHandler(this._cleanup);
  }

  /**
   * @private
   * @method readBuildFile
   * @param path The file path to read the build file from
   */
  readBuildFile(path) {
    // Load the build file
    let buildFile = findBuildFile('ember-cli-build.js', path);
    if (buildFile) {
      return buildFile({ project: this.project });
    }

    throw new SilentError('No ember-cli-build.js found.');
  }

  /**
   * @private
   * @method setupBroccoliBuilder
   */
  setupBroccoliBuilder() {
    this.environment = this.environment || 'development';
    process.env.EMBER_ENV = process.env.EMBER_ENV || this.environment;

    this.tree = this.readBuildFile(this.project.root);

    let broccoli, options = {};
    if (isExperimentEnabled('BROCCOLI_2')) {
      broccoli = require('broccoli');
      let tmpDir;

      // If not using system temp dir, compatability mode with broccoli-builder, tmp in root
      if (!isExperimentEnabled('SYSTEM_TEMP')) {
        tmpDir = `${this.project.root}/tmp`;
        if (!fs.existsSync(tmpDir)) {
          fs.mkdir(tmpDir);
        }
      }
      options = {
        tmpdir: tmpDir,
      };

      try {
        this.builder = new broccoli.Builder(this.tree, options);
        return;
      } catch (e) {
        // Catch here to trap InvalidNodeError. If this is thrown, it's because the node provided is not valid
        // and likely uses the old .read/.rebuild API, so fallback to broccoli-builder that supports that API
        if (
          !(e instanceof broccoli.Builder.InvalidNodeError) ||
          e.message.indexOf('The .read/.rebuild API is no longer supported as of Broccoli 1.0') === -1
        ) {
          throw e;
        }

        // Fallback to broccoli-builder
        let error = `Invalid Broccoli2 node detected, falling back to broccoli-builder. Broccoli error:\n`;
        error += `---------------\n`;
        error += e.message;
        error += `---------------\n`;
        this.ui.writeWarnLine(error);
      }
    } else if (isExperimentEnabled('SYSTEM_TEMP')) {
      this.ui.writeWarnLine('EMBER_CLI_SYSTEM_TEMP only works in combination with EMBER_CLI_BROCCOLI_2');
    }

    broccoli = require('broccoli-builder');
    this.broccoliBuilderFallback = true;
    this.builder = new broccoli.Builder(this.tree, options);
  }

  /**
    Determine whether the output path is safe to delete. If the outputPath
    appears anywhere in the parents of the project root, the build would
    delete the project directory. In this case return `false`, otherwise
    return `true`.

    @private
    @method canDeleteOutputPath
    @param {String} outputPath
    @return {Boolean}
  */
  canDeleteOutputPath(outputPath) {
    let rootPathParents = [this.project.root];
    let dir = path.dirname(this.project.root);
    rootPathParents.push(dir);
    while (dir !== path.dirname(dir)) {
      dir = path.dirname(dir);
      rootPathParents.push(dir);
    }
    return rootPathParents.indexOf(outputPath) === -1;
  }

  /**
   * @private
   * @method copyToOutputPath
   * @param {String} inputPath
   */
  copyToOutputPath(inputPath) {
    let outputPath = this.outputPath;

    fs.mkdirsSync(outputPath);

    if (!this.canDeleteOutputPath(outputPath)) {
      throw new SilentError(`Using a build destination path of \`${outputPath}\` is not supported.`);
    }

    let sync = this._sync;
    if (sync === undefined) {
      this._sync = sync = new Sync(inputPath, path.resolve(this.outputPath));
    }

    let changes = sync.sync();

    return changes.map(op => op[1]);
  }

  /**
   * @private
   * @method processBuildResult
   * @param results
   * @return {Promise}
   */
  processBuildResult(results) {
    return Promise.resolve()
      .then(() => this.copyToOutputPath(results.directory))
      .then(syncResult => {
        results.outputChanges = syncResult;
        return results;
      });
  }

  /**
   * @private
   * @method processAddonBuildSteps
   * @param buildStep
   * @param results
   * @return {Promise}
   */
  processAddonBuildSteps(buildStep, results) {
    let addonPromises = [];
    if (this.project && this.project.addons.length) {
      addonPromises = this.project.addons.reduce((sum, addon) => {
        let method = addon[buildStep];
        if (method) {
          let val = method.call(addon, results);
          if (val) { sum.push(val); }
        }
        return sum;
      }, []);
    }

    return Promise.all(addonPromises).then(() => results);
  }

  /**
   * @private
   * @method build
   * @return {Promise}
   */
  build(addWatchDirCallback, resultAnnotation) {
    this.project._instrumentation.start('build');

    if (!isExperimentEnabled('SYSTEM_TEMP')) {
      attemptNeverIndex('tmp');
    }

    if (addWatchDirCallback && !this.broccoliBuilderFallback) {
      for (let path of this.builder.watchedPaths) {
        addWatchDirCallback(path);
      }
    }

    return this.processAddonBuildSteps('preBuild')
      .then(() => this.builder.build(this.broccoliBuilderFallback ? addWatchDirCallback : null))
      .then(this.compatNode.bind(this), this.compatBroccoliPayload.bind(this))
      .then(this.processAddonBuildSteps.bind(this, 'postBuild'))
      .then(this.processBuildResult.bind(this))
      .then(this.processAddonBuildSteps.bind(this, 'outputReady'))
      .then(result => {
        this.project._instrumentation.stopAndReport('build', result, resultAnnotation);
        return result;
      }, reason => {
        this.project._instrumentation.stopAndReport('build', null, resultAnnotation);
        throw reason;
      })
      .then(this.checkForPostBuildEnvironmentIssues.bind(this))
      .catch(error => {
        this.processAddonBuildSteps('buildError', error);
        throw error;
      })
      .finally(this.finalizeBuild.bind(this));
  }

  /**
   * Delegates to the `cleanup` method of the wrapped Broccoli builder.
   *
   * @private
   * @method cleanup
   * @return {Promise}
   */
  cleanup() {
    if (!this._cleanupPromise) {
      let ui = this.project.ui;
      ui.startProgress('cleaning up');
      ui.writeLine('cleaning up...');

      // ensure any addon treeFor caches are reset
      _resetTreeCache();

      this._onProcessInterrupt.removeHandler(this._cleanup);

      let node = heimdall.start({ name: 'Builder Cleanup' });

      this._cleanupPromise = Promise.resolve()
        .then(() => this.builder.cleanup())
        .finally(() => {
          ui.stopProgress();
          node.stop();
        }).catch(err => {
          ui.writeLine(chalk.red('Cleanup error.'));
          ui.writeError(err);
        });
    }

    return this._cleanupPromise;
  }

  /**
   * Checks for issues in the environment that can't easily be detected until
   * after a build and issues any necessary deprecation warnings.
   *
   * - check for old (pre 0.1.4) versions of heimdalljs
   *
   * @private
   * @method checkForPostBuildEnvironmentIssues
   */
  checkForPostBuildEnvironmentIssues(value) {
    // 0.1.3 and prior used a global heimdall instance to share sessions
    // newer versions keep the session itself on process
    this.project.ui.writeDeprecateLine('Heimdalljs < 0.1.4 found.  Please remove old versions of heimdalljs and reinstall (you can find them with `npm ls heimdalljs` as long as you have nothing `npm link`d).  Performance instrumentation data will be incomplete until then.', !process._heimdall);

    return value;
  }

  /**
   * @private
   * @method finalizeBuild
   */
  finalizeBuild() {
    this.project.configCache.clear();
  }

  /**
   * broccoli-builder reformats the response into {directory, graph}, this method is a backwards
   * compatible shim for broccoli 1.x
   * @private
   * @method compatNode
   * @param node The node returned from Broccoli builder
   */
  compatNode(node) {
    if (!this.broccoliBuilderFallback) {
      return {
        directory: this.builder.outputPath,
        graph: this.builder.outputNodeWrapper,
      };
    }

    return node;
  }

  compatBroccoliPayload(err) {
    // TODO fix ember-cli/console-ui to handle current broccoli broccoliPayload
    let broccoliPayload = err && err.broccoliPayload;
    if (broccoliPayload) {
      if (!broccoliPayload.error) {
        let originalError = broccoliPayload.originalError || {};
        let location = broccoliPayload.location || originalError.location;
        broccoliPayload.error = {
          message: originalError.message,
          stack: originalError.stack,
          errorType: originalError.type || 'Build Error',
          codeFrame: originalError.codeFrame || originalError.message,
          location: location || {},
        };
      }
      if (!broccoliPayload.broccoliNode) {
        broccoliPayload.broccoliNode = {
          nodeName: broccoliPayload.nodeName,
          nodeAnnotation: broccoliPayload.nodeAnnotation,
          instantiationStack: broccoliPayload.instantiationStack || '',
        };
      }
      if (!broccoliPayload.versions) {
        let builderVersion = this.broccoliBuilderFallback
          ? require('broccoli-builder/package').version
          : require('broccoli/package').version;

        broccoliPayload.versions = {
          'broccoli-builder': builderVersion,
          node: process.version,
        };
      }
    }

    throw err;
  }
}

module.exports = Builder;
