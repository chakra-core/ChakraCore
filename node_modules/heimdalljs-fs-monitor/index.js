'use strict';

const fs = require('fs');
const heimdall = require('heimdalljs');
const logger = require('heimdalljs-logger')('heimdalljs-fs-monitor');

// It is possible for this module to be evaluated more than once in the same
// heimdall session. In that case, we need to guard against double-counting by
// making other instances of FSMonitor inert.
let isMonitorRegistrant = false;
let hasActiveInstance = false;

class FSMonitor {
  constructor() {
    this.state = 'idle';
    this.blacklist = ['createReadStream', 'createWriteStream', 'ReadStream', 'WriteStream'];
  }

  start() {
    if (isMonitorRegistrant && !hasActiveInstance) {
      this.state = 'active';
      this._attach();
      hasActiveInstance = true;
    } else {
      logger.warn('Multiple instances of heimdalljs-fs-monitor have been created'
        + ' in the same session. Since this can cause fs operations to be counted'
        + ' multiple times, this instance has been disabled.');
    }
  }

  stop() {
    if (this.state === 'active') {
      this.state = 'idle';
      this._detach();
      hasActiveInstance = false;
    }
  }

  shouldMeasure() {
    return this.state === 'active';
  }

  _measure(name, original, context, args) {
    if (this.state !== 'active') {
      throw new Error('Cannot measure if the monitor is not active');
    }

    let metrics = heimdall.statsFor('fs');
    let m = metrics[name] = metrics[name] || new Metric();

    m.start();

    // TODO: handle async
    try {
      return original.apply(context, args);
    } finally {
      m.stop();
    }
  }

  _attach() {
    let monitor = this;

    for (let member in fs) {
      if (this.blacklist.indexOf(member) === -1) {
        let old = fs[member];
        if (typeof old === 'function') {
          fs[member] = (function(old, member) {
            return function() {
              if (monitor.shouldMeasure()) {
                let args = new Array(arguments.length);
                for (let i = 0; i < arguments.length; i++) {
                  args[i] = arguments[i];
                }

                return monitor._measure(member, old, fs, args);
              } else {
                return old.apply(fs, arguments);
              }
            };
          }(old, member));

          fs[member].__restore = function() {
            fs[member] = old;
          };
        }
      }
    }
  }

  _detach() {
    for (let member in fs) {
      let maybeFunction = fs[member];
      if (typeof maybeFunction === 'function' && typeof maybeFunction.__restore === 'function') {
        maybeFunction.__restore();
      }
    }
  }
}

module.exports = FSMonitor;

if (!heimdall.hasMonitor('fs')) {
  heimdall.registerMonitor('fs', function FSSchema() {});
  isMonitorRegistrant = true;
}

class Metric {
  constructor() {
    this.count = 0;
    this.time = 0;
    this.startTime = undefined;
  }

  start() {
    this.startTime = process.hrtime();
    this.count++;
  }

  stop() {
    let now = process.hrtime();

    this.time += (now[0] - this.startTime[0]) * 1e9 + (now[1] - this.startTime[1]);
    this.startTime = undefined;
  }

  toJSON() {
    return {
      count: this.count,
      time: this.time
    };
  }
}

