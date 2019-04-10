'use strict';

const fs = require('fs-extra');
const srcURL = require('source-map-url');
const path = require('path');
const MemoryStreams = require('memory-streams');
const Coder = require('./coder');
const crypto = require('crypto');
const chalk = require('chalk');
const EOL = require('os').EOL;
const endsWith = require('./endsWith');
const validator = require('sourcemap-validator');
const logger = require('heimdalljs-logger')('fast-sourcemap-concat:');


class SourceMap {
  constructor(opts) {
    if (!opts || (!opts.outputFile && (!opts.mapURL || !opts.file))) {
      throw new Error("Must specify at least outputFile or mapURL and file");
    }
    if (opts.mapFile && !opts.mapURL) {
      throw new Error("must specify the mapURL when setting a custom mapFile");
    }
    this.baseDir = opts.baseDir;
    this.outputFile = opts.outputFile;
    this.cache = opts.cache;
    this.mapFile = opts.mapFile || (this.outputFile && this.outputFile.replace(/\.js$/, '') + '.map');
    this.mapURL = opts.mapURL || (this.mapFile && path.basename(this.mapFile));
    this.mapCommentType = opts.mapCommentType || 'line';
    this._initializeStream();
    this.id = opts.pluginId;

    this.content = {
      version: 3,
      sources: [],
      sourcesContent: [],
      names: [],
      mappings: ''
    };
    if (opts.sourceRoot) {
      this.content.sourceRoot = opts.sourceRoot;
    }
    this.content.file = opts.file || path.basename(opts.outputFile);
    this.encoder = new Coder();

    // Keep track of what column we're currently outputing in the
    // concatenated sourcecode file. Notice that we don't track line
    // though -- line is implicit in this.content.mappings.
    this.column = 0;

    // Keep track of how many lines worth of mappings we've output into
    // the concatenated sourcemap. We use this to correct broken input
    // sourcemaps that don't match the length of their sourcecode.
    this.linesMapped = 0;
    this._sizes = {};
    this.nextFileSourceNeedsComma = false;
  }

  _resolveFile(filename) {
    if (this.baseDir && !path.isAbsolute(filename)) {
      filename = path.join(this.baseDir, filename);
    }
    return filename;
  }

  _initializeStream() {
    if (this.outputFile) {
      fs.mkdirpSync(path.dirname(this.outputFile));
      this.stream = fs.createWriteStream(this.outputFile);
    } else {
      this.stream = new MemoryStreams.WritableStream();
    }
  }


  addFile(filename) {
    let source = ensurePosix(fs.readFileSync(this._resolveFile(filename), 'utf-8'));
    this._sizes[filename] = source.length;
    return this.addFileSource(filename, source);
  }

  addFileSource(filename, source, inputSrcMap) {
    let url;

    if (source.length === 0) {
      return;
    }

    if (srcURL.existsIn(source)) {
      url = srcURL.getFrom(source);
      source = srcURL.removeFrom(source);
    }

    if (this.content.mappings.length > 0 && this.nextFileSourceNeedsComma) {
      this.content.mappings += ',';
      this.nextFileSourceNeedsComma = false;
    }

    if (typeof inputSrcMap === 'string') {
      inputSrcMap = JSON.parse(inputSrcMap);
    }

    if (inputSrcMap === undefined && url) {
      inputSrcMap = this._resolveSourcemap(filename, url);
    }

    let valid = true;

    if (inputSrcMap) {
      try {
        // TODO: don't stringify here
        validator(source, JSON.stringify(inputSrcMap));
      } catch (e) {
        logger.error(' invalid sourcemap for: %s', filename);
        if (typeof e === 'object' && e !== null) {
          logger.error('   error: ', e.message);
        }

        // print
        valid = false;
      }
    }

    if (inputSrcMap && valid) {
      let haveLines = countNewLines(source);
      source = this._addMap(filename, inputSrcMap, source, haveLines);
    } else {
      logger.info('generating new map: %s', filename);
      this.content.sources.push(filename);
      this.content.sourcesContent.push(source);
      this._generateNewMap(source);
    }

    this.stream.write(source);
  }

  _cacheEncoderResults(key, operations, filename) {
    let encoderState = this.encoder.copy();
    let initialLinesMapped = this.linesMapped;
    let cacheEntry = this.cache[key];
    let finalState;

    if (cacheEntry) {
      // The cache contains the encoder-differential for our file. So
      // this updates encoderState to the final value our encoder will
      // have after processing the file.
      encoderState.decode(cacheEntry.encoder);
      // We provide that final value as a performance hint.
      operations.call(this, {
        encoder: encoderState,
        lines: cacheEntry.lines
      });

      logger.info('encoder cache hit: %s', filename);
    } else {

      // Run the operations with no hint because we don't have one yet.
      operations.call(this);
      // Then store the encoder differential in the cache.
      finalState = this.encoder.copy();
      finalState.subtract(encoderState);
      this.cache[key] = {
        encoder: finalState.serialize(),
        lines: this.linesMapped - initialLinesMapped
      };

      logger.info('encoder cache prime: %s', filename);
    }
  }

  // This is useful for things like separators that you're appending to
  // your JS file that don't need to have their own source mapping, but
  // will alter the line numbering for subsequent files.
  addSpace(source) {
    this.stream.write(source);
    let lineCount = countNewLines(source);
    if (lineCount === 0) {
      this.column += source.length;
    } else {
      this.column = 0;
      let mappings = this.content.mappings;
      for (let i = 0; i < lineCount; i++) {
        mappings += ';';
        this.nextFileSourceNeedsComma = false;
      }
      this.content.mappings = mappings;
    }
  }

  _generateNewMap(source) {
    let mappings = this.content.mappings;
    let lineCount = countNewLines(source);
    const encodedVal = this.encoder.encode({
      generatedColumn: this.column,
      source: this.content.sources.length-1,
      originalLine: 0,
      originalColumn: 0
    });

    mappings += encodedVal;
    this.nextFileSourceNeedsComma = !(endsWith(encodedVal, ';') || endsWith(encodedVal, ','));

    if (lineCount === 0) {
      // no newline in the source. Keep outputting one big line.
      this.column += source.length;
    } else {
      // end the line
      this.column = 0;
      this.encoder.resetColumn();
      mappings += ';';
      this.nextFileSourceNeedsComma = false;
      this.encoder.adjustLine(lineCount-1);
    }

    // For the remainder of the lines (if any), we're just following
    // one-to-one.
    for (let i = 0; i < lineCount-1; i++) {
      mappings += 'AACA;';
      this.nextFileSourceNeedsComma = false;
    }
    this.linesMapped += lineCount;
    this.content.mappings = mappings;
  }

  _resolveSourcemap(filename, url) {
    let srcMap;
    let match = /^data:.+?;base64,/.exec(url);

    try {
      if (match) {
        srcMap = Buffer.from(url.slice(match[0].length), 'base64');
      } else if (this.baseDir && url.slice(0,1) === '/') {
        srcMap = fs.readFileSync(
          path.join(this.baseDir, url),
          'utf8'
        );
      } else {
        srcMap = fs.readFileSync(
          path.join(path.dirname(this._resolveFile(filename)), url),
          'utf8'
        );
      }
      return JSON.parse(srcMap);
    } catch (err) {
      this._warn('Warning: ignoring input sourcemap for ' + filename + ' because ' + err.message);
    }
  }

  _addMap(filename, srcMap, source) {
    let initialLinesMapped = this.linesMapped;
    let haveLines = countNewLines(source);
    let self = this;

    if (this.cache) {
      this._cacheEncoderResults(hash(JSON.stringify(srcMap)), function(cacheHint) {
        self._assimilateExistingMap(filename, srcMap, cacheHint);
      }, filename);
    } else {
      this._assimilateExistingMap(filename, srcMap);
    }

    while (this.linesMapped - initialLinesMapped < haveLines) {
      // This cleans up after upstream sourcemaps that are too short
      // for their sourcecode so they don't break the rest of our
      // mapping. Coffeescript does this.
      this.content.mappings += ';';
      this.nextFileSourceNeedsComma = false;
      this.linesMapped++;
    }
    while (haveLines < this.linesMapped - initialLinesMapped) {
      // Likewise, this cleans up after upstream sourcemaps that are
      // too long for their sourcecode.
      source += "\n";
      haveLines++;
    }
    return source;
  }


  _assimilateExistingMap(filename, srcMap, cacheHint) {
    let content = this.content;
    let sourcesOffset = content.sources.length;
    let namesOffset = content.names.length;

    content.sources = content.sources.concat(this._resolveSources(srcMap));
    content.sourcesContent = content.sourcesContent.concat(this._resolveSourcesContent(srcMap, filename));
    while (content.sourcesContent.length > content.sources.length) {
      content.sourcesContent.pop();
    }
    while (content.sourcesContent.length < content.sources.length) {
      content.sourcesContent.push(null);
    }
    content.names = content.names.concat(srcMap.names);
    this._scanMappings(srcMap, sourcesOffset, namesOffset, cacheHint);
  }

  _resolveSources(srcMap) {
    let baseDir = this.baseDir;
    if (!baseDir) {
      return srcMap.sources;
    }
    return srcMap.sources.map(function(src) {
      return src.replace(baseDir, '');
    });
  }

  _resolveSourcesContent(srcMap, filename) {
    if (srcMap.sourcesContent) {
      // Upstream srcmap already had inline content, so easy.
      return srcMap.sourcesContent;
    } else {
      // Look for original sources relative to our input source filename.
      return srcMap.sources.map(function(source){
        let fullPath;
        if (path.isAbsolute(source)) {
          fullPath = source;
        } else {
          fullPath = path.join(path.dirname(this._resolveFile(filename)), source);
        }
        return ensurePosix(fs.readFileSync(fullPath, 'utf-8'));
      }.bind(this));
    }
  }

  _scanMappings(srcMap, sourcesOffset, namesOffset, cacheHint) {
    let mappings = this.content.mappings;
    let decoder = new Coder();
    let inputMappings = srcMap.mappings;
    let pattern = /^([;,]*)([^;,]*)/;
    let continuation = /^([;,]*)((?:AACA;)+)/;
    let initialMappedLines = this.linesMapped;
    let match;
    let lines;

    while (inputMappings.length > 0) {
      match = pattern.exec(inputMappings);

      // If the entry was preceded by separators, copy them through.
      if (match[1]) {
        mappings += match[1];
        this.nextFileSourceNeedsComma = !(endsWith(match[1], ',') || endsWith(match[1], ';'));
        lines = match[1].replace(/,/g, '').length;
        if (lines > 0) {
          this.linesMapped += lines;
          this.encoder.resetColumn();
          decoder.resetColumn();
        }
      }

      // Re-encode the entry.
      if (match[2]){
        let value = decoder.decode(match[2]);
        value.generatedColumn += this.column;
        this.column = 0;
        if (sourcesOffset && value.hasOwnProperty('source')) {
          value.source += sourcesOffset;
          decoder.prev_source += sourcesOffset;
          sourcesOffset = 0;
        }
        if (namesOffset && value.hasOwnProperty('name')) {
          value.name += namesOffset;
          decoder.prev_name += namesOffset;
          namesOffset = 0;
        }
        const encodedVal = this.encoder.encode(value);
        mappings += encodedVal;
        this.nextFileSourceNeedsComma = !(endsWith(encodedVal, ',') || endsWith(encodedVal, ';'));
      }

      inputMappings = inputMappings.slice(match[0].length);

      // Once we've applied any offsets, we can try to jump ahead.
      if (!sourcesOffset && !namesOffset) {
        if (cacheHint) {
          // Our cacheHint tells us what our final encoder state will be
          // after processing this file. And since we've got nothing
          // left ahead that needs rewriting, we can just copy the
          // remaining mappings over and jump to the final encoder
          // state.
          mappings += inputMappings;
          this.nextFileSourceNeedsComma = !(endsWith(inputMappings, ',') || endsWith(inputMappings, ';'));
          inputMappings = '';
          this.linesMapped = initialMappedLines + cacheHint.lines;
          this.encoder = cacheHint.encoder;
        }

        if ((match = continuation.exec(inputMappings))) {
          // This is a significant optimization, especially when we're
          // doing simple line-for-line concatenations.
          lines = match[2].length / 5;
          this.encoder.adjustLine(lines);
          this.encoder.resetColumn();
          decoder.adjustLine(lines);
          decoder.resetColumn();
          this.linesMapped += lines + match[1].replace(/,/g, '').length;
          mappings += match[0];
          this.nextFileSourceNeedsComma = !(endsWith(match[0], ',') || endsWith(match[0], ';'));
          inputMappings = inputMappings.slice(match[0].length);
        }
      }
    }

    // ensure we always reset the column. This is to ensure we remain resilient
    // to invalid input.
    this.encoder.resetColumn();
    this.content.mappings = mappings;
  }

  writeConcatStatsSync(outputPath, content) {
    fs.mkdirpSync(path.dirname(outputPath));
    fs.writeFileSync(outputPath, JSON.stringify(content, null, 2));
  }

  end(cb, thisArg) {
    let stream = this.stream;
    let sourceMap = this;

    return new Promise(function(resolve, reject) {
      stream.on('error', function(error) {
        stream.on('close', function() {
          reject(error);
        });

        stream.end();
      });

      let error, success;

      try {
        if (cb) {
          cb.call(thisArg, sourceMap);
        }

        if (process.env.CONCAT_STATS) {
          let concatStatsPath = process.env.CONCAT_STATS_PATH || `${process.cwd()}/concat-stats-for`;
          let outputPath = path.join(concatStatsPath, `${sourceMap.id}-${path.basename(sourceMap.outputFile)}.json`);

          sourceMap.writeConcatStatsSync(
            outputPath,
            {
              outputFile: sourceMap.outputFile,
              sizes: sourceMap._sizes
            }
          );
        }

        if (sourceMap.mapCommentType === 'line') {
          stream.write('//# sourceMappingURL=' + sourceMap.mapURL + '\n');
        } else {
          stream.write('/*# sourceMappingURL=' + sourceMap.mapURL + ' */\n');
        }

        if (sourceMap.mapFile) {
          fs.mkdirpSync(path.dirname(sourceMap.mapFile));
          fs.writeFileSync(sourceMap.mapFile, JSON.stringify(sourceMap.content));
        }
        success = true;
      } catch(e) {
        success = false;
        error = e;
      } finally {
        if (sourceMap.outputFile) {
          stream.on('close', function() {
            if (success) {
              resolve();
            } else {
              reject(error);
            }
          });
          stream.end();
        } else {
          resolve({
            code: stream.toString(),
            map: sourceMap.content
          });
        }
      }
    });
  }

  _warn(msg) {
    console.warn(chalk.yellow(msg));
  }
}

module.exports = SourceMap;

function countNewLines(src) {
  let newlinePattern = /(\r?\n)/g;
  let count = 0;
  while (newlinePattern.exec(src)) {
    count++;
  }
  return count;
}


function hash(string) {
  return crypto.createHash('md5').update(string).digest('hex');
}

function ensurePosix(string) {
  if (EOL !== '\n') {
    string = string.split(EOL).join('\n');
  }
  return string;
}
