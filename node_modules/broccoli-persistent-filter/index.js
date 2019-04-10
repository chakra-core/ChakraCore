'use strict';

const fs = require('fs');
const path = require('path');
const mkdirp = require('mkdirp');
const rimraf = require('rimraf');
const Promise = require('rsvp').Promise;
const Plugin = require('broccoli-plugin');
const walkSync = require('walk-sync');
const mapSeries = require('promise-map-series');
const symlinkOrCopySync = require('symlink-or-copy').sync;
const debugGenerator = require('heimdalljs-logger');
const md5Hex = require('./lib/md5-hex');
const Processor = require('./lib/processor');
const defaultProccessor = require('./lib/strategies/default');
const hashForDep = require('hash-for-dep');
const FSTree = require('fs-tree-diff');
const heimdall = require('heimdalljs');
const queue = require('async-promise-queue');

class ApplyPatchesSchema {
  constructor() {
    this.mkdir = 0;
    this.rmdir = 0;
    this.unlink = 0;
    this.change = 0;
    this.create = 0;
    this.other = 0;
    this.processed = 0;
    this.linked = 0;
    this.handleFile = 0;

    this.processString = 0;
    this.processStringTime = 0;
    this.persistentCacheHit = 0;
    this.persistentCachePrime = 0;
    this.handleFileTime = 0;
  }
}

class DerivePatchesSchema {
  constructor() {
    this.patches = 0;
    this.entries = 0;
  }
}

const worker = queue.async.asyncify(doWork => doWork());

module.exports = Filter;

module.exports.shouldPersist = shouldPersist;

function shouldPersist(env, persist) {
  let result;

  if (env.CI) {
    result = persist && env.FORCE_PERSISTENCE_IN_CI;
  } else {
    result = persist;
  }

  return !!result;
}

Filter.prototype = Object.create(Plugin.prototype);
Filter.prototype.constructor = Filter;

function Filter(inputTree, options) {
  if (!this || !(this instanceof Filter) ||
      Object.getPrototypeOf(this) === Filter.prototype) {
    throw new TypeError('Filter is an abstract class and must be sub-classed');
  }

  let loggerName = 'broccoli-persistent-filter:' + (this.constructor.name);
  let annotation = (options && options.annotation) || this.annotation || this.description;

  if (annotation) {
    loggerName += ' > [' + annotation + ']';
  }

  this._logger = debugGenerator(loggerName);

  Plugin.call(this, [inputTree], {
    name: (options && options.name) || this.name || loggerName,
    annotation: (options && options.annotation) || this.annotation || annotation,
    persistentOutput: true
  });

  this.processor = new Processor(options);
  this.processor.setStrategy(defaultProccessor);
  this.currentTree = new FSTree();

  /* Destructuring assignment in node 0.12.2 would be really handy for this! */
  if (options) {
    if (options.extensions != null)      this.extensions = options.extensions;
    if (options.targetExtension != null) this.targetExtension = options.targetExtension;
    if (options.inputEncoding != null)   this.inputEncoding = options.inputEncoding;
    if (options.outputEncoding != null)  this.outputEncoding = options.outputEncoding;
    if (shouldPersist(process.env, options.persist)) {
      this.processor.setStrategy(require('./lib/strategies/persistent'));
    }
    this.async = (options.async === true);
  }

  this.processor.init(this);

  this._canProcessCache = Object.create(null);
  this._destFilePathCache = Object.create(null);
  this._needsReset = false;

  this.concurrency = (options && options.concurrency) || Number(process.env.JOBS) || Math.max(require('os').cpus().length - 1, 1);
  this._outputLinks = Object.create(null);
}

function nanosecondsSince(time) {
  let delta = process.hrtime(time);
  return delta[0] * 1e9 + delta[1];
}

function timeSince(time) {
  let deltaNS = nanosecondsSince(time);
  return (deltaNS / 1e6).toFixed(2) +' ms';
}

Filter.prototype.build = function() {
  if (this._needsReset) {
    this.currentTree = new FSTree();
    rimraf.sync(this.outputPath);
    mkdirp.sync(this.outputPath);
  }

  this._needsReset = true;

  let srcDir = this.inputPaths[0];
  let destDir = this.outputPath;

  let prevTime = process.hrtime();
  let instrumentation = heimdall.start('derivePatches', DerivePatchesSchema);

  let walkStart = process.hrtime();
  let entries = walkSync.entries(srcDir);
  let walkDuration = timeSince(walkStart);

  let nextTree = FSTree.fromEntries(entries);
  let currentTree = this.currentTree;

  this.currentTree = nextTree;

  let patches = currentTree.calculatePatch(nextTree);

  instrumentation.stats.patches = patches.length;
  instrumentation.stats.entries = entries.length;
  instrumentation.stats.walk = {
    entries: entries.length,
    duration: walkDuration
  };

  this._logger.info('derivePatches', 'duration:', timeSince(prevTime), JSON.stringify(instrumentation.stats));

  instrumentation.stop();

  // used with options.async = true to allow 'create' and 'change' operations to complete async
  const pendingWork = [];

  return heimdall.node('applyPatches', ApplyPatchesSchema, instrumentation => {
    let prevTime = process.hrtime();
    return mapSeries(patches, patch => {
      let operation = patch[0];
      let relativePath = patch[1];
      let entry = patch[2];
      let outputPath = destDir + '/' + (this.getDestFilePath(relativePath, entry) || relativePath);
      let outputFilePath = outputPath;

      this._logger.debug('[operation:%s] %s', operation, relativePath);

      switch (operation) {
        case 'mkdir': {
          instrumentation.mkdir++;
          return fs.mkdirSync(outputPath);
        } case 'rmdir': {
          instrumentation.rmdir++;
          return fs.rmdirSync(outputPath);
        } case 'unlink': {
          instrumentation.unlink++;
          return fs.unlinkSync(outputPath);
        } case 'change': {
          // wrap this in a function so it doesn't actually run yet, and can be throttled
          let changeOperation = () => {
            instrumentation.change++;
            return this._handleFile(relativePath, srcDir, destDir, entry, outputFilePath, true, instrumentation);
          };
          if (this.async) {
            pendingWork.push(changeOperation);
            return;
          }
          return changeOperation();
        } case 'create': {
          // wrap this in a function so it doesn't actually run yet, and can be throttled
          let createOperation = () => {
            instrumentation.create++;
            return this._handleFile(relativePath, srcDir, destDir, entry, outputFilePath, false, instrumentation);
          };
          if (this.async) {
            pendingWork.push(createOperation);
            return;
          }
          return createOperation();
        } default: {
          instrumentation.other++;
        }
      }
    }).then(() => {
      return queue(worker, pendingWork, this.concurrency);
    }).then((result) => {
      this._logger.info('applyPatches', 'duration:', timeSince(prevTime), JSON.stringify(instrumentation));
      this._needsReset = false;
      return result;
    });
  });
};

Filter.prototype._handleFile = function(relativePath, srcDir, destDir, entry, outputPath, isChange, stats) {
  stats.handleFile++;

  let handleFileStart = process.hrtime();

  return new Promise(resolve => {
    let result;
    let srcPath = srcDir + '/' + relativePath;

    if (this.canProcessFile(relativePath, entry)) {
      stats.processed++;
      if (this._outputLinks[outputPath] === true) {
        delete this._outputLinks[outputPath];
        fs.unlinkSync(outputPath);
      }
      result = this.processAndCacheFile(srcDir, destDir, entry, isChange, stats);
    } else {
      stats.linked++;
      if (isChange) {
        fs.unlinkSync(outputPath);
      }
      result = symlinkOrCopySync(srcPath, outputPath);
      this._outputLinks[outputPath] = true;
    }
    resolve(result);
  }).then(val => {
    stats.handleFileTime += nanosecondsSince(handleFileStart);
    return val;
  }, err => {
    stats.handleFileTime += nanosecondsSince(handleFileStart);
    throw err;
  });
};

/*
 The cache key to be used for this plugins set of dependencies. By default
 a hash is created based on `package.json` and nested dependencies.

 Implement this to customize the cache key (for example if you need to
 account for non-NPM dependencies).

 @public
 @method cacheKey
 @returns {String}
 */
Filter.prototype.cacheKey = function() {
  return hashForDep(this.baseDir());
};

/* @public
 *
 * @method baseDir
 * @returns {String} absolute path to the root of the filter...
 */
Filter.prototype.baseDir = function() {
  throw Error('Filter must implement prototype.baseDir');
};

/**
 * @public
 *
 * optionally override this to build a more rhobust cache key
 * @param  {String} string The contents of a file that is being processed
 * @return {String}        A cache key
 */
Filter.prototype.cacheKeyProcessString = function(string, relativePath) {
  return md5Hex(string + 0x00 + relativePath);
};

Filter.prototype.canProcessFile =
    function canProcessFile(relativePath, entry) {
  return !!this.getDestFilePath(relativePath, entry);
};

Filter.prototype.isDirectory = function(relativePath, entry) {
  if (this.inputPaths === undefined) {
    return false;
  }

  let srcDir = this.inputPaths[0];
  let path = srcDir + '/' + relativePath;

  return (entry || fs.lstatSync(path)).isDirectory();
};

Filter.prototype.getDestFilePath = function(relativePath, entry) {
  // NOTE: relativePath may have been moved or unlinked
  if (this.isDirectory(relativePath, entry)) {
    return null;
  }

  if (this.extensions == null) {
    return relativePath;
  }

  for (let i = 0, ii = this.extensions.length; i < ii; ++i) {
    let ext = this.extensions[i];
    if (relativePath.slice(-ext.length - 1) === '.' + ext) {
      if (this.targetExtension != null) {
        relativePath = relativePath.slice(0, -ext.length) + this.targetExtension;
      }
      return relativePath;
    }
  }

  return null;
};

Filter.prototype.processAndCacheFile = function(srcDir, destDir, entry, isChange, instrumentation) {
  let filter = this;
  let relativePath = entry.relativePath;

  return Promise.resolve().
    then(() => {
      return filter.processFile(srcDir, destDir, relativePath, isChange, instrumentation, entry);
    }).
    then(undefined,
      // TODO(@caitp): error wrapper is for API compat, but is not particularly
      // useful.
      // istanbul ignore next
      e => {
        if (typeof e !== 'object') e = new Error('' + e);
        e.file = relativePath;
        e.treeDir = srcDir;
        throw e;
      });
};

function invoke(context, fn, args) {
  return new Promise(resolve => {
    resolve(fn.apply(context, args));
  });
}

Filter.prototype.processFile = function(srcDir, destDir, relativePath, isChange, instrumentation, entry) {
  let filter = this;
  let inputEncoding = this.inputEncoding;
  let outputEncoding = this.outputEncoding;

  if (inputEncoding === undefined)  inputEncoding  = 'utf8';
  if (outputEncoding === undefined) outputEncoding = 'utf8';

  let contents = fs.readFileSync(srcDir + '/' + relativePath, {
    encoding: inputEncoding
  });

  instrumentation.processString++;
  let processStringStart = process.hrtime();
  let string = invoke(this.processor, this.processor.processString, [this, contents, relativePath, instrumentation]);

  return string.then(outputString => {
    instrumentation.processStringTime += nanosecondsSince(processStringStart);
    let outputPath = filter.getDestFilePath(relativePath, entry);

    if (outputPath == null) {
      throw new Error('canProcessFile("' + relativePath +
                      '") is true, but getDestFilePath("' +
                      relativePath + '") is null');
    }

    outputPath = destDir + '/' + outputPath;

    if (isChange) {
      let isSame = fs.readFileSync(outputPath, 'UTF-8') === outputString;
      if (isSame) {

        this._logger.debug('[change:%s] but was the same, skipping', relativePath, isSame);
        return;
      } else {
        this._logger.debug('[change:%s] but was NOT the same, writing new file', relativePath);
      }
    }

    try {
      fs.writeFileSync(outputPath, outputString, {
        encoding: outputEncoding
      });

    } catch(e) {
      // optimistically assume the DIR was patched correctly
      mkdirp.sync(path.dirname(outputPath));
      fs.writeFileSync(outputPath, outputString, {
        encoding: outputEncoding
      });
    }

    return outputString;
  });
};

Filter.prototype.processString = function(/* contents, relativePath */) {
  throw new Error(
      'When subclassing broccoli-persistent-filter you must implement the ' +
      '`processString()` method.');
};

Filter.prototype.postProcess = function(result /*, relativePath */) {
  return result;
};
