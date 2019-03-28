'use strict';

const debug = require('debug')('log4js:fileSync');
const path = require('path');
const fs = require('fs');
const os = require('os');

const eol = os.EOL || '\n';

function touchFile(file, options) {
  // if the file exists, nothing to do
  if (fs.existsSync(file)) {
    return;
  }

  // touch the file to apply flags (like w to truncate the file)
  const id = fs.openSync(file, options.flags, options.mode);
  fs.closeSync(id);
}

class RollingFileSync {
  constructor(filename, size, backups, options) {
    debug('In RollingFileStream');

    function throwErrorIfArgumentsAreNotValid() {
      if (!filename || !size || size <= 0) {
        throw new Error('You must specify a filename and file size');
      }
    }

    throwErrorIfArgumentsAreNotValid();

    this.filename = filename;
    this.size = size;
    this.backups = backups || 1;
    this.options = options;
    this.currentSize = 0;

    function currentFileSize(file) {
      let fileSize = 0;

      try {
        fileSize = fs.statSync(file).size;
      } catch (e) {
        // file does not exist
        touchFile(file, options);
      }
      return fileSize;
    }

    this.currentSize = currentFileSize(this.filename);
  }

  shouldRoll() {
    debug('should roll with current size %d, and max size %d', this.currentSize, this.size);
    return this.currentSize >= this.size;
  }

  roll(filename) {
    const that = this;
    const nameMatcher = new RegExp(`^${path.basename(filename)}`);

    function justTheseFiles(item) {
      return nameMatcher.test(item);
    }

    function index(filename_) {
      return parseInt(filename_.substring((`${path.basename(filename)}.`).length), 10) || 0;
    }

    function byIndex(a, b) {
      if (index(a) > index(b)) {
        return 1;
      } else if (index(a) < index(b)) {
        return -1;
      }

      return 0;
    }

    function increaseFileIndex(fileToRename) {
      const idx = index(fileToRename);
      debug(`Index of ${fileToRename} is ${idx}`);
      if (idx < that.backups) {
        // on windows, you can get a EEXIST error if you rename a file to an existing file
        // so, we'll try to delete the file we're renaming to first
        try {
          fs.unlinkSync(`${filename}.${idx + 1}`);
        } catch (e) {
          // ignore err: if we could not delete, it's most likely that it doesn't exist
        }

        debug(`Renaming ${fileToRename} -> ${filename}.${idx + 1}`);
        fs.renameSync(path.join(path.dirname(filename), fileToRename), `${filename}.${idx + 1}`);
      }
    }

    function renameTheFiles() {
      // roll the backups (rename file.n to file.n+1, where n <= numBackups)
      debug('Renaming the old files');

      const files = fs.readdirSync(path.dirname(filename));
      files.filter(justTheseFiles).sort(byIndex).reverse().forEach(increaseFileIndex);
    }

    debug('Rolling, rolling, rolling');
    renameTheFiles();
  }

  /* eslint no-unused-vars:0 */
  write(chunk, encoding) {
    const that = this;


    function writeTheChunk() {
      debug('writing the chunk to the file');
      that.currentSize += chunk.length;
      fs.appendFileSync(that.filename, chunk);
    }

    debug('in write');


    if (this.shouldRoll()) {
      this.currentSize = 0;
      this.roll(this.filename);
    }

    writeTheChunk();
  }
}

/**
 * File Appender writing the logs to a text file. Supports rolling of logs by size.
 *
 * @param file file log messages will be written to
 * @param layout a function that takes a logevent and returns a string
 *   (defaults to basicLayout).
 * @param logSize - the maximum size (in bytes) for a log file,
 *   if not provided then logs won't be rotated.
 * @param numBackups - the number of log files to keep after logSize
 *   has been reached (default 5)
 * @param timezoneOffset - optional timezone offset in minutes
 *   (default system local)
 * @param options - passed as is to fs options
 */
function fileAppender(file, layout, logSize, numBackups, timezoneOffset, options) {
  debug('fileSync appender created');
  file = path.normalize(file);
  numBackups = numBackups === undefined ? 5 : numBackups;
  // there has to be at least one backup if logSize has been specified
  numBackups = numBackups === 0 ? 1 : numBackups;

  function openTheStream(filePath, fileSize, numFiles) {
    let stream;

    if (fileSize) {
      stream = new RollingFileSync(
        filePath,
        fileSize,
        numFiles,
        options
      );
    } else {
      stream = (((f) => {
        // touch the file to apply flags (like w to truncate the file)
        touchFile(f, options);

        return {
          write(data) {
            fs.appendFileSync(f, data);
          }
        };
      }))(filePath);
    }

    return stream;
  }

  const logFile = openTheStream(file, logSize, numBackups);

  return (loggingEvent) => {
    logFile.write(layout(loggingEvent, timezoneOffset) + eol);
  };
}

function configure(config, layouts) {
  let layout = layouts.basicLayout;
  if (config.layout) {
    layout = layouts.layout(config.layout.type, config.layout);
  }

  const options = {
    flags: config.flags || 'a',
    encoding: config.encoding || 'utf8',
    mode: config.mode || 0o644
  };

  return fileAppender(
    config.filename,
    layout,
    config.maxLogSize,
    config.backups,
    config.timezoneOffset,
    options
  );
}

module.exports.configure = configure;
