'use strict';

const ora              = require('ora');
const EOL              = require('os').EOL;
const chalk            = require('chalk');
const writeError       = require('./write-error');
const tmpdir           = require('os').tmpdir();
const crypto           = require('crypto');
const fs               = require('fs');

const DEFAULT_WRITE_LEVEL = 'INFO';

/**
  The UI provides the CLI with a unified mechanism for providing output and
  requesting input from the user. This becomes useful when wanting to adjust
  logLevels, or mock input/output for tests.

  @class UI
  @constructor
  @param {Object} options Configuration options

  @example
  ```js
  new UI({
    inputStream: process.stdin,
    outputStream: process.stdout,
    writeLevel: 'DEBUG' | 'INFO' | 'WARNING' | 'ERROR',
    ci: true | false
  });
  ```
*/
class UI {
  constructor(_options) {
    let spinner = this.spinner = ora({ color: 'green' });
    let options = _options || {};
    this.through = require('through');
    // Output stream
    this.actualOutputStream = options.outputStream || process.stdout;
    this.outputStream = this.through(function (data) {
      spinner.stop();
      this.emit('data', data);
    });
    this.outputStream.setMaxListeners(0);
    this.outputStream.pipe(this.actualOutputStream);
    this.inputStream = options.inputStream || process.stdin;
    this.errorStream = options.errorStream || process.stderr;
    this.errorLog = options.errorLog || [];
    this.writeLevel = options.writeLevel || DEFAULT_WRITE_LEVEL;
    this.errorReport = null;
    this.ci = !!options.ci;
  }
  /**
    Unified mechanism to write a string to the console.
    Optionally include a writeLevel, this is used to decide if the specific
    logging mechanism should or should not be printed.

    @method write
    @param {String} data
    @param {String} [writeLevel='INFO']
  */
  write(data, writeLevel) {
    if (writeLevel === 'ERROR') {
      this.errorStream.write(data);
    } else if (this.writeLevelVisible(writeLevel)) {
      this.outputStream.write(data);
    }
  }
  /**
    Unified mechanism to write a string and new line to the console.
    Optionally include a writeLevel, this is used to decide if the specific
    logging mechanism should or should not be printed.
    @method writeLine
    @param {String} data
    @param {String} [writeLevel='INFO']
  */
  writeLine(data, writeLevel) {
    this.write(data + EOL, writeLevel);
  }
  /**
    Helper method to write a string with the DEBUG writeLevel and gray chalk
    @method writeDebugLine
    @param {String} data
  */
  writeDebugLine(data) {
    this.writeLine(chalk.gray(data), 'DEBUG');
  }
  /**
    Helper method to write a string with the INFO writeLevel and cyan chalk
    @method writeInfoLine
    @param {String} data
  */
  writeInfoLine(data) {
    this.writeLine(chalk.cyan(data), 'INFO');
  }
  /**
    Helper method to write a string with the WARNING writeLevel and yellow chalk.
    Optionally include a test. If falsy, the warning will be printed. By default, warnings
    will be prepended with WARNING text when printed.
    @method writeWarnLine
    @param {String} data
    @param {Boolean} test
    @param {Boolean} prepend
  */
  writeWarnLine(data, test, prepend) {
    if (test) {
      return;
    }

    data = this.prependLine('WARNING', data, prepend);
    this.writeLine(chalk.yellow(data), 'WARNING', test);
  }
  /**
    Helper method to write a string with the WARNING writeLevel and yellow chalk.
    Optionally include a test. If falsy, the deprecation will be printed. By default deprecations
    will be prepended with DEPRECATION text when printed.
    @method writeDeprecateLine
    @param {String} data
    @param {Boolean} test
    @param {Boolean} prepend
  */
  writeDeprecateLine(data, test, prepend) {
    data = this.prependLine('DEPRECATION', data, prepend);
    this.writeWarnLine(data, test, false);
  }

  /**
    Utility method to prepend a line with a flag-like string (i.e., WARNING).
    @method prependLine
    @param {String} prependData
    @param {String} data
    @param {Boolean} prepend
  */
  prependLine(prependData, data, prepend) {
    if (typeof prepend === 'undefined' || prepend) {
      data = prependData + ': ' + data;
    }

    return data;
  }

  /**
    Unified mechanism to an Error to the console.
    This will occure at a writeLevel of ERROR

    @method writeError
    @param {Error} error
  */
  writeError(error) {
    const report = writeError(this, error);
    if (report === null) {
      return;
    }

    if (typeof error === 'object' && error !== null && !error.isSilentError) {
      if (process.env.CI) {
        // In CI mode we print the error details to the console directly
        this.writeLine(report, 'ERROR');
      } else {
        // In non-CI mode we write the error details to a file that can be read later
        const md5 = crypto.createHash('md5').update(report).digest("hex");
        const filepath = `${tmpdir}/error.dump.${md5}.log`;
        fs.writeFileSync(filepath, report);
        this.writeLine(chalk.red(`\nStack Trace and Error Report: ${filepath}`), 'ERROR');
      }
    }
  }

  /**
    Sets the write level for the UI. Valid write levels are 'DEBUG', 'INFO',
    'WARNING', and 'ERROR'.

    @method setWriteLevel
    @param {String} level
  */
  setWriteLevel(level) {
    if (Object.keys(this.WRITE_LEVELS).indexOf(level) === -1) {
      throw new Error('Unknown write level. Valid values are \'DEBUG\', \'INFO\', \'WARNING\', and \'ERROR\'.');
    }

    this.writeLevel = level;
  }

  startProgress(message /*, stepString*/) {
    if (this.writeLevelVisible('INFO')) {
      if (this.ci) {
        this.writeLine(message);
      } else {
        this.spinner.text = message;
        this.spinner.start();
      }
    }
  }

  stopProgress() {
    if (this.writeLevelVisible('INFO') && !this.ci) {
      this.spinner.stop();
    }
  }

  /**
    Launch the prompt interface (inquiry session) with (Array of Questions || Question)

    See [Inquirer.js#question](https://github.com/SBoudrias/Inquirer.js#question) for Question {Object} properties

    @method prompt
    @param {Object} questions
    @param {Function} [callback]
    @return {Promise}

  */
  prompt(questions, callback) {
    let inquirer = require('inquirer');
    return inquirer.prompt(questions).then(function (answer) {
      if (callback) {
        callback(answer);
      }

      return answer;
    });
  }
  /**
    Whether or not the specified write level should be printed by this UI.

    @method writeLevelVisible
    @private
    @param {String} writeLevel
    @return {Boolean}
  */
  writeLevelVisible(writeLevel) {
    let levels = this.WRITE_LEVELS;
    writeLevel = writeLevel || DEFAULT_WRITE_LEVEL;
    return levels[writeLevel] >= levels[this.writeLevel];
  }
}

/**
  @property WRITE_LEVELS
  @private
  @type Object
*/
UI.prototype.WRITE_LEVELS = {
  'DEBUG': 1,
  'INFO': 2,
  'WARNING': 3,
  'ERROR': 4
};

// Note: You should use `ui.outputStream`, `ui.inputStream` and `ui.write()`
//       instead of `process.stdout` and `console.log`.
//       Thus the pleasant progress indicator automatically gets
//       interrupted and doesn't mess up the output! -> Convenience :P
module.exports = UI;
