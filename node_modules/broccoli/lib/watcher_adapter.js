'use strict';

const EventEmitter = require('events').EventEmitter;
const sane = require('sane');
const logger = require('heimdalljs-logger')('broccoli:watcherAdapter');

function defaultFilterFunction(name) {
  return /^[^.]/.test(name);
}

function bindFileEvent(adapter, watcher, event) {
  watcher.on(event, (filepath, root) => {
    logger.debug(event, root + '/' + filepath);
    adapter.emit('change');
  });
}

module.exports = class WatcherAdapter extends EventEmitter {
  constructor(options) {
    super();
    this.options = options || {};
    this.options.filter = this.options.filter || defaultFilterFunction;
    this.watchers = [];
  }

  watch(watchedPaths) {
    if (!Array.isArray(watchedPaths)) {
      throw new TypeError(`WatcherAdapter#watch's first argument must be an array of watchedPaths`);
    }
    let watchers = watchedPaths.map(watchedPath => {
      const watcher = new sane(watchedPath, this.options);
      this.watchers.push(watcher);
      bindFileEvent(this, watcher, 'change');
      bindFileEvent(this, watcher, 'add');
      bindFileEvent(this, watcher, 'delete');
      return new Promise((resolve, reject) => {
        watcher.on('ready', resolve);
        watcher.on('error', reject);
      }).then(() => {
        watcher.removeAllListeners('ready');
        watcher.removeAllListeners('error');
        watcher.on('error', err => {
          logger.debug('error', err);
          this.emit('error', err);
        });
        logger.debug('ready', watchedPath);
      });
    });
    return Promise.all(watchers).then(() => {});
  }

  quit() {
    let closing = this.watchers.map(
      watcher =>
        new Promise((resolve, reject) =>
          watcher.close(err => {
            if (err) reject(err);
            else resolve();
          })
        )
    );
    this.watchers.length = 0;
    return Promise.all(closing).then(() => {});
  }
};

module.exports.bindFileEvent = bindFileEvent;
