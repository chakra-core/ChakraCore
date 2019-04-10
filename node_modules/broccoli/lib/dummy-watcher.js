'use strict';

const EventEmitter = require('events').EventEmitter;

module.exports = class Watcher extends EventEmitter {
  constructor(builder) {
    super();
    this.builder = builder;
    this.currentBuild = null;
    this._lifetimeDeferred = null;
  }

  start() {
    let lifetime = (this._lifetimeDeferred = {});
    lifetime.promise = new Promise((resolve, reject) => {
      lifetime.resolve = resolve;
      lifetime.reject = reject;
    });

    this.currentBuild = this.builder.build();
    this.currentBuild
      .then(() => this.emit('buildSuccess'))
      .catch(err => this.emit('buildFailure', err));
    return this._lifetimeDeferred.promise;
  }

  quit() {
    this._lifetimeDeferred.resolve();
  }
};
