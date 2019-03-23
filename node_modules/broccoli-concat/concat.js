'use strict';

const Plugin = require('broccoli-plugin');
const FSTree = require('fs-tree-diff');
const path = require('path');
const fs = require('fs-extra');
const merge = require('lodash.merge');
const omit = require('lodash.omit');
const uniq = require('lodash.uniq');
const walkSync = require('walk-sync');
const ensurePosix = require('ensure-posix-path');

const StatsOutput = require('./lib/stats-output');
const ensureNoGlob = require('./lib/utils/ensure-no-glob');
const isDirectory = require('./lib/utils/is-directory');
const makeIndex = require('./lib/utils/make-index');

let id = 0;

module.exports = class Concat extends Plugin {
  constructor(inputNode, options, Strategy) {
    if (!options || !options.outputFile) {
      throw new Error('the outputFile option is required');
    }

    id++;
    let inputNodes = Concat.inputNodesForConcatStats(inputNode, id, options.outputFile);

    super(inputNodes, {
      annotation: options.annotation,
      name: (Strategy.name || 'Unknown') + 'Concat',
      persistentOutput: true
    });

    this.id = id;

    if (Strategy === undefined) {
      throw new TypeError('Concat requires a concat Strategy');
    }

    this.Strategy = Strategy;
    this.sourceMapConfig = omit(options.sourceMapConfig || {}, 'enabled');
    this.allInputFiles = uniq([].concat(options.headerFiles || [], options.inputFiles || [], options.footerFiles || []));
    this.inputFiles = options.inputFiles;
    this.outputFile = options.outputFile;
    this.allowNone = options.allowNone;
    this.header = options.header;
    this.headerFiles = options.headerFiles;
    this._headerFooterFilesIndex = makeIndex(options.headerFiles, options.footerFiles);
    this.footer = options.footer;
    this.footerFiles = options.footerFiles;
    this.separator = (options.separator != null) ? options.separator : '\n';
    this.contentLimit = options.contentLimit;

    ensureNoGlob('headerFiles', this.headerFiles);
    ensureNoGlob('footerFiles', this.footerFiles);

    this._lastTree = FSTree.fromEntries([]);
    this._hasBuilt = false;
    this._hasStatsOutput = false;

    this.encoderCache = {};
  }

  static inputNodesForConcatStats(inputNode, id, outputFile) {
    let dir = Concat.concatStatsPath();

    return [
      new StatsOutput(inputNode, {
        dir,
        name: id + '-' + path.basename(outputFile)
      })
    ];
  }

  static concatStatsPath() {
    return process.env.CONCAT_STATS_PATH || `${process.cwd()}/concat-stats-for`;
  }

  calculatePatch() {
    let currentTree = this.getCurrentFSTree();
    let patch = this._lastTree.calculatePatch(currentTree);

    this._lastTree = currentTree;

    return patch;
  }

  build() {
    let patch = this.calculatePatch();

    // We skip building if this is a rebuild with a zero-length patch
    // and the request for stats output has not changed since the last build
    if (patch.length === 0 && this._hasBuilt && (!process.env.CONCAT_STATS || this._hasStatsOutput)) {
      return;
    }

    this._hasBuilt = true;
    if (process.env.CONCAT_STATS) {
      fs.mkdirpSync(Concat.concatStatsPath());
      this._hasStatsOutput = true;
    }

    if (this.Strategy.isPatchBased) {
      return this._doPatchBasedBuild(patch);
    } else {
      return this._doLegacyBuild();
    }
  }

  _doPatchBasedBuild(patch) {
    if (!this.concat) {
      this.concat = new this.Strategy(merge(this.sourceMapConfig, {
        separator: this.separator,
        header: this.header,
        headerFiles: this.headerFiles,
        footerFiles: this.footerFiles,
        footer: this.footer,
        contentLimit: this.contentLimit,
        baseDir: this.inputPaths[0]
      }));
    }

    for (let i = 0; i < patch.length; i++) {
      let operation = patch[i];
      let method = operation[0];
      let file = operation[1];

      switch (method) {
        case 'create':
          this.concat.addFile(file, this._readFile(file));
          break;
        case 'change':
          this.concat.updateFile(file, this._readFile(file));
          break;
        case 'unlink':
          this.concat.removeFile(file);
          break;
      }
    }

    let outputFile = path.join(this.outputPath, this.outputFile);
    let content = this.concat.result();

    // If content is undefined, then we the concat had no input files
    if (content === undefined) {
      if (!this.allowNone) {
        throw new Error('Concat: nothing matched [' + this.inputFiles + ']');
      } else {
        content = '';
      }
    }

    if (process.env.CONCAT_STATS) {
      let fileSizes = this.concat.fileSizes();
      let statsPath = Concat.concatStatsPath();
      let outputPath = path.join(statsPath, `${this.id}-${path.basename(this.outputFile)}.json`);

      fs.mkdirpSync(statsPath);
      fs.writeFileSync(outputPath, JSON.stringify({
        outputFile: this.outputFile,
        sizes: fileSizes
      }, null, 2));
    }

    fs.outputFileSync(outputFile, content);
  }

  _readFile(file) {
    return fs.readFileSync(path.join(this.inputPaths[0], file), 'UTF-8');
  }

  _doLegacyBuild() {
    let separator = this.separator;
    let firstSection = true;
    let outputFile = path.join(this.outputPath, this.outputFile);

    fs.mkdirpSync(path.dirname(outputFile));

    this.concat = new this.Strategy(merge(this.sourceMapConfig, {
      outputFile: outputFile,
      baseDir: this.inputPaths[0],
      cache: this.encoderCache,
      pluginId: this.id
    }));

    return this.concat.end(concat => {
      function beginSection() {
        if (firstSection) {
          firstSection = false;
        } else {
          concat.addSpace(separator);
        }
      }

      if (this.header) {
        beginSection();
        concat.addSpace(this.header);
      }

      if (this.headerFiles) {
        this.headerFiles.forEach(file => {
          beginSection();
          concat.addFile(file);
        });
      }

      this.addFiles(beginSection);

      if (this.footerFiles) {
        this.footerFiles.forEach(file => {
          beginSection();
          concat.addFile(file);
        });
      }

      if (this.footer) {
        beginSection();
        concat.addSpace(this.footer + '\n');
      }
    }, this);
  }

  getCurrentFSTree() {
    return FSTree.fromEntries(this.listEntries());
  }

  listEntries() {
    // If we have no inputFiles at all, use undefined as the filter to return
    // all files in the inputDir.
    let filter = this.allInputFiles.length ? this.allInputFiles : undefined;
    let inputDir = this.inputPaths[0];
    return walkSync.entries(inputDir, filter);
  }

  /**
   * Returns the full paths for any matching inputFiles.
   */
  listFiles() {
    let inputDir = this.inputPaths[0];
    return this.listEntries().map(function(entry) {
      return ensurePosix(path.join(inputDir, entry.relativePath));
    });
  }

  addFiles(beginSection) {
    let headerFooterFileOverlap = false;
    let posixInputPath = ensurePosix(this.inputPaths[0]);

    let files = this.listFiles().filter(file => {
      let relativePath = file.replace(posixInputPath + '/', '');

      // * remove inputFiles that are already contained within headerFiles and footerFiles
      // * allow duplicates between headerFiles and footerFiles

      if (this._headerFooterFilesIndex[relativePath] === true) {
        headerFooterFileOverlap = true;
        return false;
      }

      return !isDirectory(file);
    }, this);

    // raise IFF:
    //   * headerFiles or footerFiles overlapped with inputFiles
    //   * nothing matched inputFiles
    if (headerFooterFileOverlap === false &&
        files.length === 0 &&
        !this.allowNone) {
      throw new Error('Concat: nothing matched [' + this.inputFiles + ']');
    }

    files.forEach(file => {
      beginSection();
      this.concat.addFile(file.replace(posixInputPath + '/', ''));
    }, this);
  }
};
