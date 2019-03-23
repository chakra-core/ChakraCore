'use strict';

const fs = require('fs');
const path = require('path-posix');
const mkdirp = require('mkdirp');
const walkSync = require('walk-sync');
const Minimatch = require('minimatch').Minimatch;
const arrayEqual = require('array-equal');
const Plugin = require('broccoli-plugin');
const symlinkOrCopy = require('symlink-or-copy');
const debug = require('debug');
const FSTree = require('fs-tree-diff');
const rimraf = require('rimraf');
const BlankObject = require('blank-object');
const heimdall = require('heimdalljs');

function ApplyPatchesSchema() {
  this.mkdir = 0;
  this.rmdir = 0;
  this.unlink = 0;
  this.change = 0;
  this.create = 0;
  this.other = 0;
  this.processed = 0;
  this.linked = 0;
}

function makeDictionary() {
  let cache = new BlankObject();

  cache['_dict'] = null;
  delete cache['_dict'];
  return cache;
}
// copied mostly from node-glob cc @isaacs
function isNotAPattern(pattern) {
  let set = new Minimatch(pattern).set;
  if (set.length > 1) {
    return false;
  }

  for (let j = 0; j < set[0].length; j++) {
    if (typeof set[0][j] !== 'string') {
      return false;
    }
  }

  return true;
}

Funnel.prototype = Object.create(Plugin.prototype);
Funnel.prototype.constructor = Funnel;
function Funnel(inputNode, _options) {
  if (!(this instanceof Funnel)) { return new Funnel(inputNode, _options); }

  let options = _options || {};
  Plugin.call(this, [inputNode], {
    annotation: options.annotation,
    persistentOutput: true,
    needsCache: false,
  });

  this._includeFileCache = makeDictionary();
  this._destinationPathCache = makeDictionary();
  this._currentTree = new FSTree();
  this._isRebuild = false;

  let keys = Object.keys(options || {});
  for (let i = 0, l = keys.length; i < l; i++) {
    let key = keys[i];
    this[key] = options[key];
  }

  this.destDir = this.destDir || '/';
  this.count = 0;

  if (this.files && typeof this.files === 'function') {
    // Save dynamic files func as a different variable and let the rest of the code
    // still assume that this.files is always an array.
    this._dynamicFilesFunc = this.files;
    delete this.files;
  } else if (this.files && !Array.isArray(this.files)) {
    throw new Error('Invalid files option, it must be an array or function (that returns an array).');
  }

  if ((this.files || this._dynamicFilesFunc) && (this.include || this.exclude)) {
    throw new Error('Cannot pass files option (array or function) and a include/exlude filter. You can have one or the other');
  }

  if (this.files) {
    if (this.files.filter(isNotAPattern).length !== this.files.length) {
      console.warn('broccoli-funnel does not support `files:` option with globs, please use `include:` instead');
      this.include = this.files;
      this.files = undefined;
    }
  }

  this._setupFilter('include');
  this._setupFilter('exclude');

  this._matchedWalk = this.canMatchWalk();

  this._instantiatedStack = (new Error()).stack;
  this._buildStart = undefined;
}

function isMinimatch(x) {
  return x instanceof Minimatch;
}
Funnel.prototype.canMatchWalk = function() {
  let include = this.include;
  let exclude = this.exclude;

  if (!include && !exclude) { return false; }

  let includeIsOk = true;

  if (include) {
    includeIsOk = include.filter(isMinimatch).length === include.length;
  }

  let excludeIsOk = true;

  if (exclude) {
    excludeIsOk = exclude.filter(isMinimatch).length === exclude.length;
  }

  return includeIsOk && excludeIsOk;
};

Funnel.prototype._debugName = function() {
  return this.description || this._annotation || this.name || this.constructor.name;
};

Funnel.prototype._debug = function() {
  debug(`broccoli-funnel:${this._debugName()}`).apply(null, arguments);
};

Funnel.prototype._setupFilter = function(type) {
  if (!this[type]) {
    return;
  }

  if (!Array.isArray(this[type])) {
    throw new Error(`Invalid ${type} option, it must be an array. You specified \`${typeof this[type]}\`.`);
  }

  // Clone the filter array so we are not mutating an external variable
  let filters = this[type] = this[type].slice(0);

  for (let i = 0, l = filters.length; i < l; i++) {
    filters[i] = this._processPattern(filters[i]);
  }
};

Funnel.prototype._processPattern = function(pattern) {
  if (pattern instanceof RegExp) {
    return pattern;
  }

  let type = typeof pattern;

  if (type === 'string') {
    return new Minimatch(pattern);
  }

  if (type === 'function') {
    return pattern;
  }

  throw new Error(`include/exclude patterns can be a RegExp, glob string, or function. You supplied \`${typeof pattern}\`.`);
};

Funnel.prototype.shouldLinkRoots = function() {
  return !this.files && !this.include && !this.exclude && !this.getDestinationPath;
};

Funnel.prototype.build = function() {
  this._buildStart = new Date();
  this.destPath = path.join(this.outputPath, this.destDir);

  if (this.destPath[this.destPath.length - 1] === '/') {
    this.destPath = this.destPath.slice(0, -1);
  }

  let inputPath = this.inputPaths[0];
  if (this.srcDir) {
    inputPath = path.join(inputPath, this.srcDir);
  }

  if (this._dynamicFilesFunc) {
    this.lastFiles = this.files;
    this.files = this._dynamicFilesFunc() || [];

    // Blow away the include cache if the list of files is new
    if (this.lastFiles !== undefined && !arrayEqual(this.lastFiles, this.files)) {
      this._includeFileCache = makeDictionary();
    }
  }

  let inputPathExists = fs.existsSync(inputPath);

  let linkedRoots = false;
  if (this.shouldLinkRoots()) {
    linkedRoots = true;

    /**
     * We want to link the roots of these directories, but there are a few
     * edge cases we must account for.
     *
     * 1. It's possible that the original input doesn't actually exist.
     * 2. It's possible that the output symlink has been broken.
     * 3. We need slightly different behavior on rebuilds.
     *
     * Behavior has been modified to always having an `else` clause so that
     * the code is forced to account for all scenarios. Not accounting for
     * all scenarios made it possible for initial builds to succeed without
     * specifying `this.allowEmpty`.
     */

    // This is specifically looking for broken symlinks.
    let outputPathExists = fs.existsSync(this.outputPath);

    // Doesn't count as a rebuild if there's not an existing outputPath.
    this._isRebuild = this._isRebuild && outputPathExists;

    /*eslint-disable no-lonely-if*/
    if (this._isRebuild) {
      if (inputPathExists) {
        // Already works because of symlinks. Do nothing.
      } else if (!inputPathExists && this.allowEmpty) {
        // Make sure we're safely using a new outputPath since we were previously symlinked:
        rimraf.sync(this.outputPath);
        // Create a new empty folder:
        mkdirp.sync(this.destPath);
      } else { // this._isRebuild && !inputPathExists && !this.allowEmpty
        // Need to remove it on the rebuild.
        // Can blindly remove a symlink if path exists.
        rimraf.sync(this.outputPath);
      }
    } else { // Not a rebuild.
      if (inputPathExists) {
        // We don't want to use the generated-for-us folder.
        // Instead let's remove it:
        rimraf.sync(this.outputPath);
        // And then symlinkOrCopy over top of it:
        this._copy(inputPath, this.destPath);
      } else if (!inputPathExists && this.allowEmpty) {
        // Can't symlink nothing, so make an empty folder at `destPath`:
        mkdirp.sync(this.destPath);
      } else { // !this._isRebuild && !inputPathExists && !this.allowEmpty
        throw new Error(`You specified a \`"srcDir": ${this.srcDir}\` which does not exist and did not specify \`"allowEmpty": true\`.`);
      }
    }
    /*eslint-enable no-lonely-if*/

    this._isRebuild = true;
  } else if (inputPathExists) {
    this.processFilters(inputPath);
  } else if (!this.allowEmpty) {
    throw new Error(`You specified a \`"srcDir": ${this.srcDir}\` which does not exist and did not specify \`"allowEmpty": true\`.`);
  } else { // !inputPathExists && this.allowEmpty
    // Just make an empty folder so that any downstream consumers who don't know
    // to ignore this on `allowEmpty` don't get trolled.
    mkdirp(this.destPath);
  }

  this._debug('build, %o', {
    in: `${new Date() - this._buildStart}ms`,
    linkedRoots,
    inputPath,
    destPath: this.destPath,
  });
};

function ensureRelative(string) {
  if (string.charAt(0) === '/') {
    return string.substring(1);
  }
  return string;
}

Funnel.prototype._processEntries = function(entries) {
  return entries.filter(function(entry) {
    // support the second set of filters walk-sync does not support
    //   * regexp
    //   * excludes
    return this.includeFile(entry.relativePath);
  }, this).map(function(entry) {

    let relativePath = entry.relativePath;

    entry.relativePath = this.lookupDestinationPath(relativePath);

    this.outputToInputMappings[entry.relativePath] = relativePath;

    return entry;
  }, this);
};

Funnel.prototype._processPaths = function(paths) {
  return paths.
    slice(0).
    filter(this.includeFile, this).
    map(function(relativePath) {
      let output = this.lookupDestinationPath(relativePath);
      this.outputToInputMappings[output] = relativePath;
      return output;
    }, this);
};

Funnel.prototype.processFilters = function(inputPath) {
  let nextTree;

  let instrumentation = heimdall.start('derivePatches');
  let entries;

  this.outputToInputMappings = {}; // we allow users to rename files

  if (this.files && !this.exclude && !this.include) {
    entries = this._processPaths(this.files);
    // clone to be compatible with walkSync
    nextTree = FSTree.fromPaths(entries, { sortAndExpand: true });
  } else {

    if (this._matchedWalk) {
      entries = walkSync.entries(inputPath, { globs: this.include, ignore: this.exclude });
    } else {
      entries = walkSync.entries(inputPath);
    }

    entries = this._processEntries(entries);
    nextTree = FSTree.fromEntries(entries, { sortAndExpand: true });
  }

  let patches = this._currentTree.calculatePatch(nextTree);

  this._currentTree = nextTree;

  instrumentation.stats.patches = patches.length;
  instrumentation.stats.entries = entries.length;

  let outputPath = this.outputPath;

  instrumentation.stop();

  instrumentation = heimdall.start('applyPatch', ApplyPatchesSchema);

  patches.forEach(function(entry) {
    this._applyPatch(entry, inputPath, outputPath, instrumentation.stats);
  }, this);

  instrumentation.stop();
};

Funnel.prototype._applyPatch = function applyPatch(entry, inputPath, _outputPath, stats) {
  let outputToInput = this.outputToInputMappings;
  let operation = entry[0];
  let outputRelative = entry[1];

  if (!outputRelative) {
    // broccoli itself maintains the roots, we can skip any operation on them
    return;
  }

  let outputPath = `${_outputPath}/${outputRelative}`;

  this._debug('%s %s', operation, outputPath);

  switch (operation) {
    case 'unlink' :
      stats.unlink++;

      fs.unlinkSync(outputPath);
      break;
    case 'rmdir' :
      stats.rmdir++;
      fs.rmdirSync(outputPath);
      break;
    case 'mkdir' :
      stats.mkdir++;
      fs.mkdirSync(outputPath);
      break;
    case 'change':
      stats.change++;
      /* falls through */
    case 'create': {
      if (operation === 'create') {
        stats.create++;
      }

      let relativePath = outputToInput[outputRelative];
      if (relativePath === undefined) {
        relativePath = outputToInput[`/${outputRelative}`];
      }
      this.processFile(`${inputPath}/${relativePath}`, outputPath, relativePath);
      break;
    }
    default: throw new Error(`Unknown operation: ${operation}`);
  }
};

Funnel.prototype.lookupDestinationPath = function(relativePath) {
  if (this._destinationPathCache[relativePath] !== undefined) {
    return this._destinationPathCache[relativePath];
  }

  // the destDir is absolute to prevent '..' above the output dir
  if (this.getDestinationPath) {
    return this._destinationPathCache[relativePath] = ensureRelative(path.join(this.destDir, this.getDestinationPath(relativePath)));
  }

  return this._destinationPathCache[relativePath] = ensureRelative(path.join(this.destDir, relativePath));
};

Funnel.prototype.includeFile = function(relativePath) {
  let includeFileCache = this._includeFileCache;

  if (includeFileCache[relativePath] !== undefined) {
    return includeFileCache[relativePath];
  }

  // do not include directories, only files
  if (relativePath[relativePath.length - 1] === '/') {
    return includeFileCache[relativePath] = false;
  }

  let i, l, pattern;

  // Check for specific files listing
  if (this.files) {
    return includeFileCache[relativePath] = this.files.indexOf(relativePath) > -1;
  }

  if (this._matchedWalk) {
    return true;
  }
  // Check exclude patterns
  if (this.exclude) {
    for (i = 0, l = this.exclude.length; i < l; i++) {
      // An exclude pattern that returns true should be ignored
      pattern = this.exclude[i];

      if (this._matchesPattern(pattern, relativePath)) {
        return includeFileCache[relativePath] = false;
      }
    }
  }

  // Check include patterns
  if (this.include && this.include.length > 0) {
    for (i = 0, l = this.include.length; i < l; i++) {
      // An include pattern that returns true (and wasn't excluded at all)
      // should _not_ be ignored
      pattern = this.include[i];

      if (this._matchesPattern(pattern, relativePath)) {
        return includeFileCache[relativePath] = true;
      }
    }

    // If no include patterns were matched, ignore this file.
    return includeFileCache[relativePath] = false;
  }

  // Otherwise, don't ignore this file
  return includeFileCache[relativePath] = true;
};

Funnel.prototype._matchesPattern = function(pattern, relativePath) {
  if (pattern instanceof RegExp) {
    return pattern.test(relativePath);
  } else if (pattern instanceof Minimatch) {
    return pattern.match(relativePath);
  } else if (typeof pattern === 'function') {
    return pattern(relativePath);
  }

  throw new Error(`Pattern \`${pattern}\` was not a RegExp, Glob, or Function.`);
};

Funnel.prototype.processFile = function(sourcePath, destPath /*, relativePath */) {
  this._copy(sourcePath, destPath);
};

Funnel.prototype._copy = function(sourcePath, destPath) {
  let destDir = path.dirname(destPath);

  try {
    symlinkOrCopy.sync(sourcePath, destPath);
  } catch (e) {
    if (!fs.existsSync(destDir)) {
      mkdirp.sync(destDir);
    }
    try {
      fs.unlinkSync(destPath);
    } catch (e) {
      // swallow the error
    }
    symlinkOrCopy.sync(sourcePath, destPath);
  }
};

module.exports = Funnel;
