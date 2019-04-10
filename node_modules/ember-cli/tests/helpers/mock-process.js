'use strict';

let EventEmitter = require('events');

module.exports = class MockProcess extends EventEmitter {
  constructor(options) {
    super();

    options = options || {};

    this.platform = 'MockOS';

    const stdin = Object.assign(new EventEmitter(), {
      isRaw: process.stdin.isRaw,
      setRawMode: flag => {
        stdin.isRaw = flag;
      },
    }, options.stdin || {});

    Object.assign(this, options, { stdin });
  }

  getSignalListenerCounts() {
    return {
      SIGINT: this.listenerCount('SIGINT'),
      SIGTERM: this.listenerCount('SIGTERM'),
      message: this.listenerCount('message'),
    };
  }

  exit() {
    // we are unable to reliable unit test `process.exit()`
    throw new Error("MockProcess.exit() was called");
  }
};
