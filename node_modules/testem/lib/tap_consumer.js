'use strict';

const yaml = require('js-yaml');
const extend = require('util')._extend;
const EventEmitter = require('events').EventEmitter;
const TapParser = require('tap-parser');
const log = require('npmlog');

module.exports = class TapConsumer extends EventEmitter {
  constructor() {
    super();

    this.stream = new TapParser();
    this.stream.on('assert', this.onTapAssert.bind(this));
    this.stream.on('extra', this.onTapExtra.bind(this));

    this.stream.on('complete', this.onTapEnd.bind(this));
    this.stream.on('bailout', this.onTapError.bind(this));
  }

  onTapAssert(data) {
    log.info(data);
    if (data.skip) {
      return;
    }

    if (data.id === undefined) {
      return;
    }

    let test = {
      passed: 0,
      failed: 0,
      total: 1,
      id: data.id,
      name: data.name ? data.name.trim() : '',
      items: []
    };

    if (!data.ok) {
      let stack;
      if (data.diag) {
        stack = data.diag.stack || data.diag.at;
      }
      if (stack) {
        stack = yaml.dump(stack);
      }
      data = extend(data, data.diag);

      this.latestItem = extend(data, {
        passed: false,
        stack: stack
      });
      test.items.push(this.latestItem);
      test.failed++;
    } else {
      test.passed++;
    }
    this.emit('test-result', test);
  }

  onTapExtra(extra) {
    if (!this.latestItem) {
      return;
    }

    if (this.latestItem.stack) {
      this.latestItem.stack += extra;
    } else {
      this.latestItem.stack = extra;
    }
  }

  onTapError(reason) {
    let test = {
      failed: 1,
      name: 'bailout',
      items: [],
      error: {
        message: reason
      }
    };

    this.stream.removeAllListeners();
    this.emit('test-result', test);
    this.emit('all-test-results');
  }

  onTapEnd() {
    this.stream.removeAllListeners();
    this.emit('all-test-results');
  }
};
