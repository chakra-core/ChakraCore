'use strict';

const indent = require('../strutils').indent;
const printf = require('printf');

module.exports = class DotReporter {
  constructor(silent, out) {
    this.out = out || process.stdout;
    this.silent = silent;
    this.stoppedOnError = null;
    this.id = 1;
    this.total = 0;
    this.pass = 0;
    this.skipped = 0;
    this.results = [];
    this.startTime = new Date();
    this.endTime = null;
    this.currentLineChars = 0;
    this.maxLineChars = Math.min(this.out.columns || 65, 65) - 5;
    this.out.write('\n');
    this.out.write('  ');
  }

  report(prefix, data) {
    this.results.push({
      launcher: prefix,
      result: data
    });
    this.display(prefix, data);
    this.total++;
    if (data.skipped) {
      this.skipped++;
    } else if (data.passed) {
      this.pass++;
    }
  }

  display(prefix, result) {
    if (this.silent) {
      return;
    }
    if (this.currentLineChars > this.maxLineChars) {
      this.currentLineChars = 0;
      this.out.write('\n  ');
    }
    if (result.passed) {
      this.out.write('.');
    } else if (result.skipped) {
      this.out.write('*');
    } else {
      this.out.write('F');
    }
    this.currentLineChars += 1;
  }

  finish() {
    if (this.silent) {
      return;
    }
    this.endTime = new Date();
    this.out.write('\n\n');
    this.out.write(this.summaryDisplay());
    this.out.write('\n\n');
    this.displayErrors();
  }

  displayErrors() {
    this.results.forEach((data, idx) => {
      let result = data.result;
      let error = result.error;
      if (!error) {
        return;
      }

      printf(this.out, '%*d) [%s] %s\n', idx + 1, 3, data.launcher, result.name);

      if (error.message) {
        printf(this.out, '     %s\n', error.message);
      }

      if ('expected' in error && 'actual' in error) {
        printf(this.out, '\n' +
               '     expected: %s%O\n' +
               '       actual: %O\n', (error.negative ? 'NOT ' : ''), error.expected, error.actual);
      }

      if (error.stack) {
        printf(this.out, '\n%s', indent(error.stack, 5));
      }

      this.out.write('\n\n');
    }, this);
  }

  summaryDisplay() {
    return printf('  %d tests complete (%d ms)', this.total, this.duration());
  }

  duration() {
    return Math.round((this.endTime - this.startTime));
  }
};
