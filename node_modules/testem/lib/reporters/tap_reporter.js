'use strict';

const displayutils = require('../displayutils');

module.exports = class TapReporter {
  constructor(silent, out, config) {
    this.out = out || process.stdout;
    this.silent = silent;
    this.quietLogs = !!config.get('tap_quiet_logs');
    this.stoppedOnError = null;
    this.id = 1;
    this.total = 0;
    this.pass = 0;
    this.skipped = 0;
    this.results = [];
    this.errors = [];
    this.logs = [];
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

  summaryDisplay() {
    let lines = [
      '1..' + this.total,
      '# tests ' + this.total,
      '# pass  ' + this.pass,
      '# skip  ' + this.skipped,
      '# fail  ' + (this.total - this.pass - this.skipped)
    ];

    if (this.pass + this.skipped === this.total) {
      lines.push('');
      lines.push('# ok');
    }
    return lines.join('\n');
  }

  display(prefix, result) {
    if (this.silent) {
      return;
    }
    this.out.write(displayutils.resultString(this.id++, prefix, result, this.quietLogs));
  }

  finish() {
    if (this.silent) {
      return;
    }
    this.out.write('\n' + this.summaryDisplay() + '\n');
  }
};
