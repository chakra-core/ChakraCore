'use strict';

module.exports = {
  get Builder() {
    return require('./builder');
  },
  get loadBrocfile() {
    return require('./load_brocfile');
  },
  get server() {
    return require('./server');
  },
  get getMiddleware() {
    return require('./middleware');
  },
  get Watcher() {
    return require('./watcher');
  },
  get WatcherAdapter() {
    return require('./watcher_adapter');
  },
  get cli() {
    return require('./cli');
  },
};
