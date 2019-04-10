'use strict';

var utils = require('./utils');

/**
 * `Run` represents a single execution of a `Task`.
 *
 * @param {Number} `id` identifier of this run.
 * @api public
 */

function Run(id) {
  if (!(this instanceof Run)) {
    return new Run(id);
  }
  this.runId = id;
  this.date = {};
  this.hr = {};

  /**
   * Calculate the difference between the `start` and `end` hr times in nanoseconds.
   *
   * @api public
   * @name hr.diff
   */

  Object.defineProperty(this.hr, 'diff', {
    get: function() {
      return utils.nano(this.end) - utils.nano(this.start);
    }
  });

  /**
   * Calculate the offset between the hr `duration` and hr `diff` properties in nanoseconds.
   * This is needed because `duration` is called with `process.hrtime(hr.start)` after `hr.end`
   * is calculated using `process.hrtime()`.
   *
   * @api public
   * @name hr.offset
   */

  Object.defineProperty(this.hr, 'offset', {
    get: function() {
      return utils.nano(this.duration) - this.diff;
    }
  });
}

/**
 * Start recording the run times. This will save the start date on `run.date.start` and the start hr time on `run.hr.start`
 */

Run.prototype.start = function() {
  this.date.start = new Date();
  this.hr.start = process.hrtime();
};

/**
 * Stop recording the run times. This will save the end hr time on `run.hr.end`,
 * calculate the duration using `process.hrtime(run.hr.start)`,
 * and save the end date on `run.date.end`
 *
 * `end` is calculated before `duration` causing `duration` to be approximately 10,000 nanoseconds off.
 * See `offset` for actual `offset`
 */

Run.prototype.end = function() {
  this.hr.end = process.hrtime();
  this.hr.duration = process.hrtime(this.hr.start);
  this.date.end = new Date();
};

/**
 * Expose `Run`
 */

module.exports = Run;
