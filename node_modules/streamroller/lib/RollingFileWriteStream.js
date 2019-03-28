const debug = require('debug')('streamroller:RollingFileWriteStream');
const _ = require('lodash');
const async = require('async');
const fs = require('fs-extra');
const zlib = require('zlib');
const path = require('path');
const newNow = require('./now');
const format = require('date-format');
const { Writable } = require('stream');

const FILENAME_SEP = '.';
const ZIP_EXT = '.gz';

const moveAndMaybeCompressFile = (sourceFilePath, targetFilePath, needCompress, done) => {
  if (sourceFilePath === targetFilePath) {
    debug(`moveAndMaybeCompressFile: source and target are the same, not doing anything`);
    return done();
  }
  fs.access(sourceFilePath, fs.constants.W_OK | fs.constants.R_OK,  (e) => {
    if (e) {
      debug(`moveAndMaybeCompressFile: source file path does not exist. not moving. sourceFilePath=${sourceFilePath}`);
      return done();
    }

    debug(`moveAndMaybeCompressFile: moving file from ${sourceFilePath} to ${targetFilePath} ${needCompress ? 'with' : 'without'} compress`);
    if (needCompress) {
      fs.createReadStream(sourceFilePath)
        .pipe(zlib.createGzip())
        .pipe(fs.createWriteStream(targetFilePath))
        .on('finish', () => {
          debug(`moveAndMaybeCompressFile: finished compressing ${targetFilePath}, deleting ${sourceFilePath}`);
          fs.unlink(sourceFilePath, done);
        });
    } else {
      debug(`moveAndMaybeCompressFile: deleting file=${targetFilePath}, renaming ${sourceFilePath} to ${targetFilePath}`);
      fs.unlink(targetFilePath, () => { fs.rename(sourceFilePath, targetFilePath, done); });
    }
  });
};


/**
 * RollingFileWriteStream is mainly used when writing to a file rolling by date or size.
 * RollingFileWriteStream inhebites from stream.Writable
 */
class RollingFileWriteStream extends Writable {
  /**
   * Create a RollingFileWriteStream
   * @constructor
   * @param {string} filePath - The file path to write.
   * @param {object} options - The extra options
   * @param {number} options.numToKeep - The max numbers of files to keep.
   * @param {number} options.maxSize - The maxSize one file can reach. Unit is Byte.
   *                                   This should be more than 1024. The default is Number.MAX_SAFE_INTEGER.
   * @param {string} options.mode - The mode of the files. The default is '0644'. Refer to stream.writable for more.
   * @param {string} options.flags - The default is 'a'. Refer to stream.flags for more.
   * @param {boolean} options.compress - Whether to compress backup files.
   * @param {boolean} options.keepFileExt - Whether to keep the file extension.
   * @param {string} options.pattern - The date string pattern in the file name.
   * @param {boolean} options.alwaysIncludePattern - Whether to add date to the name of the first file.
   */
  constructor(filePath, options) {
    debug(`creating RollingFileWriteStream. path=${filePath}`);
    super(options);
    this.options = this._parseOption(options);
    this.fileObject = path.parse(filePath);
    if (this.fileObject.dir === '') {
      this.fileObject = path.parse(path.join(process.cwd(), filePath));
    }
    this.justTheFile = this._formatFileName({isHotFile: true});
    this.filename = path.join(this.fileObject.dir, this.justTheFile);
    this.state = {
      currentDate: newNow(),
      currentIndex: 0,
      currentSize: 0
    };
    debug(`create new file with no hot file. name=${this.justTheFile}, state=${JSON.stringify(this.state)}`);
    this._renewWriteStream();

  }

  _parseOption(rawOptions) {
    const defaultOptions = {
      maxSize: Number.MAX_SAFE_INTEGER,
      numToKeep: Number.MAX_SAFE_INTEGER,
      encoding: 'utf8',
      mode: parseInt('0644', 8),
      flags: 'a',
      compress: false,
      keepFileExt: false,
      alwaysIncludePattern: false
    };
    const options = _.defaults({}, rawOptions, defaultOptions);
    if (options.maxSize <= 0) {
      throw new Error(`options.maxSize (${options.maxSize}) should be > 0`);
    }
    if (options.numToKeep < 1) {
      throw new Error(`options.numToKeep (${options.numToKeep}) should be > 0`);
    }
    debug(`creating stream with option=${JSON.stringify(options)}`);
    return options;
  }

  _shouldRoll(callback) {
      debug(`in _shouldRoll, pattern = ${this.options.pattern}, currentDate = ${this.state.currentDate}, now = ${newNow()}`);
      if (this.options.pattern && (format(this.options.pattern, this.state.currentDate) !== format(this.options.pattern, newNow()))) {
        this._roll({isNextPeriod: true}, callback);
        return;
      }
      debug(`in _shouldRoll, currentSize = ${this.state.currentSize}, maxSize = ${this.options.maxSize}`);
      if (this.state.currentSize >= this.options.maxSize) {
        this._roll({isNextPeriod: false}, callback);
        return;
      }
      callback();
  }

  _write(chunk, encoding, callback) {
    this._shouldRoll(() => {
      debug(`writing chunk. ` +
        `file=${this.currentFileStream.path} ` +
        `state=${JSON.stringify(this.state)} ` +
        `chunk=${chunk}`
      );
      this.currentFileStream.write(chunk, encoding, (e) => {
        this.state.currentSize += chunk.length;
        callback(e);
      });
    });
  }

  // Sorted from the oldest to the latest
  _getExistingFiles(cb) {
    fs.readdir(this.fileObject.dir, (e, files) => {
      debug(`_getExistingFiles: files=${files}`);
      const existingFileDetails = _.compact(
        _.map(files, n => {
          const parseResult = this._parseFileName(n);
          debug(`_getExistingFiles: parsed ${n} as ${parseResult}`);
          if (!parseResult) {
            return;
          }
          return _.assign({fileName: n}, parseResult);
        })
      );
      cb(null, _.sortBy(
        existingFileDetails,
        n => (n.date ? n.date.valueOf() : newNow().valueOf()) - n.index
      ));
    });
  }

  // need file name instead of file abs path.
  _parseFileName(fileName) {
    let isCompressed = false;
    if (fileName.endsWith(ZIP_EXT)) {
      fileName = fileName.slice(0, -1 * ZIP_EXT.length);
      isCompressed = true;
    }
    let metaStr;
    if (this.options.keepFileExt) {
      const prefix = this.fileObject.name + FILENAME_SEP;
      const suffix = this.fileObject.ext;
      if (!fileName.startsWith(prefix) || !fileName.endsWith(suffix)) {
        return;
      }
      metaStr = fileName.slice(prefix.length, -1 * suffix.length);
      debug(`metaStr=${metaStr}, fileName=${fileName}, prefix=${prefix}, suffix=${suffix}`);
    } else {
      const prefix = this.fileObject.base;
      if (!fileName.startsWith(prefix)) {
        return;
      }
      metaStr = fileName.slice(prefix.length);
      debug(`metaStr=${metaStr}, fileName=${fileName}, prefix=${prefix}`);
    }
    if (!metaStr) {
      return {
        index: 0,
        isCompressed
      };
    }
    if (this.options.pattern) {
      const items = _.split(metaStr, FILENAME_SEP);
      const indexStr = items[items.length - 1];
      debug('items: ', items, ', indexStr: ', indexStr);
      if (indexStr !== undefined && indexStr.match(/^\d+$/)) {
        const dateStr = metaStr.slice(0, -1 * (indexStr.length + 1));
        debug(`dateStr is ${dateStr}`);
        if (dateStr) {
          return {
              index: parseInt(indexStr, 10),
              date: format.parse(this.options.pattern, dateStr),
              isCompressed
            };
        }
      }
      debug(`metaStr is ${metaStr}`);
      return {
        index: 0,
        date: format.parse(this.options.pattern, metaStr),
        isCompressed
      };
    } else {
      if (metaStr.match(/^\d+$/)) {
        return {
          index: parseInt(metaStr, 10),
          isCompressed
        };
      }
    }
    return;
  }

  _formatFileName({date, index, isHotFile}) {
    debug(`_formatFileName: date=${date}, index=${index}, isHotFile=${isHotFile}`);
    const dateOpt = date || _.get(this, 'state.currentDate') || newNow();
    const dateStr = format(this.options.pattern, dateOpt);
    const indexOpt = index || _.get(this, 'state.currentIndex');
    const oriFileName = this.fileObject.base;
    if (isHotFile) {
      debug(`_formatFileName: includePattern? ${this.options.alwaysIncludePattern}, pattern: ${this.options.pattern}`);
      if (this.options.alwaysIncludePattern && this.options.pattern) {
        debug(`_formatFileName: is hot file, and include pattern, so: ${oriFileName + FILENAME_SEP + dateStr}`);
        return this.options.keepFileExt
          ? this.fileObject.name + FILENAME_SEP + dateStr + this.fileObject.ext
          : oriFileName + FILENAME_SEP + dateStr;
      }
      debug(`_formatFileName: is hot file so, filename: ${oriFileName}`);
      return oriFileName;
    }
    let fileNameExtraItems = [];
    if (this.options.pattern) {
      fileNameExtraItems.push(dateStr);
    }
    if (indexOpt && this.options.maxSize < Number.MAX_SAFE_INTEGER) {
      fileNameExtraItems.push(indexOpt);
    }
    let fileName;
    if (this.options.keepFileExt) {
      const baseFileName = this.fileObject.name + FILENAME_SEP + fileNameExtraItems.join(FILENAME_SEP);
      fileName = baseFileName + this.fileObject.ext;
    } else {
      fileName = oriFileName + FILENAME_SEP + fileNameExtraItems.join(FILENAME_SEP);
    }
    if (this.options.compress) {
      fileName += ZIP_EXT;
    }
    debug(`_formatFileName: ${fileName}`);
    return fileName;
  }

  _moveOldFiles(isNextPeriod, cb) {
    const currentFilePath = this.currentFileStream.path;
    debug(`currentIndex = ${this.state.currentIndex}, numToKeep = ${this.options.numToKeep}`);
    let totalFilesToMove = _.min([this.state.currentIndex, this.options.numToKeep - 1]);
    const filesToMove = [];
    for (let i = totalFilesToMove; i >= 0; i--) {
      debug(`i = ${i}`);
      const sourceFilePath = i === 0
        ? currentFilePath
        : path.format({
          dir: this.fileObject.dir,
          base: this._formatFileName({date: this.state.currentDate, index: i})
        });
      const targetFilePath = i === 0 && isNextPeriod
        ? path.format({
          dir: this.fileObject.dir,
          base: this._formatFileName({date: this.state.currentDate, index: 1 })
        })
        : path.format({
          dir: this.fileObject.dir,
          base: this._formatFileName({date: this.state.currentDate, index: i + 1})
        });
      filesToMove.push({ sourceFilePath, targetFilePath });
    }
    async.eachOfSeries(filesToMove, (files, idx, cb1) => {
      debug(`src=${files.sourceFilePath}, tgt=${files.sourceFilePath}, idx=${idx}, pos=${filesToMove.length -1 -idx}`);
      moveAndMaybeCompressFile(files.sourceFilePath, files.targetFilePath, this.options.compress && (filesToMove.length -1 -idx) === 0, cb1);
    }, () => {
        if (isNextPeriod) {
          this.state.currentSize = 0;
          this.state.currentIndex = 0;
          this.state.currentDate = newNow();
          debug(`rolling for next period. state=${JSON.stringify(this.state)}`);
        } else {
          this.state.currentSize = 0;
          this.state.currentIndex += 1;
          debug(`rolling during the same period. state=${JSON.stringify(this.state)}`);
        }
        this._renewWriteStream();
        // wait for the file to be open before cleaning up old ones,
        // otherwise the daysToKeep calculations can be off
        this.currentFileStream.write('', 'utf8', () => this._clean(cb));
      });
  }

  _roll({isNextPeriod}, cb) {
    debug(`rolling, isNextPeriod ? ${isNextPeriod}`);
    debug(`_roll: closing the current stream`);
    this.currentFileStream.end('', this.options.encoding, () => { this._moveOldFiles(isNextPeriod, cb); });
  }

  _renewWriteStream() {
    fs.ensureDirSync(this.fileObject.dir);
    this.justTheFile = this._formatFileName({ date: this.state.currentDate, index: this.state.index, isHotFile: true });
    const filePath = path.format({dir: this.fileObject.dir, base: this.justTheFile});
    const ops = _.pick(this.options, ['flags', 'encoding', 'mode']);
    this.currentFileStream = fs.createWriteStream(filePath, ops);
    this.currentFileStream.on('error', (e) => { this.emit('error', e); });
  }

  _clean(cb) {
    this._getExistingFiles((e, existingFileDetails) => {
      debug(`numToKeep = ${this.options.numToKeep}, existingFiles = ${existingFileDetails.length}`);
      debug('existing files are: ', existingFileDetails);
      if (existingFileDetails.length > this.options.numToKeep) {
        const fileNamesToRemove = _.slice(
          existingFileDetails.map(f => f.fileName),
          0,
          existingFileDetails.length - this.options.numToKeep - 1
        );
        this._deleteFiles(fileNamesToRemove, () => {
          this.state.currentIndex = Math.min(this.state.currentIndex, this.options.numToKeep -1);
          cb();
        });
        return;
      }
      cb();
    });
  }

  _deleteFiles(fileNames, done) {
    debug(`files to delete: ${fileNames}`);
    async.each(
      _.map(fileNames, f => path.format({ dir: this.fileObject.dir, base: f})),
      fs.unlink,
      done
    );
    return;
  }
}

module.exports = RollingFileWriteStream;
