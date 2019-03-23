'use strict';

const debug = require('debug')('log4js:multiFile');
const path = require('path');
const fileAppender = require('./file');

const findFileKey = (property, event) => event[property] || event.context[property];

module.exports.configure = (config, layouts) => {
  debug('Creating a multi-file appender');
  const files = new Map();
  const timers = new Map();

  function checkForTimeout(fileKey) {
    const timer = timers.get(fileKey);
    const app = files.get(fileKey);
    if (timer && app) {
      if (Date.now() - timer.lastUsed > timer.timeout) {
        debug('%s not used for > %d ms => close', fileKey, timer.timeout);
        clearInterval(timer.interval);
        timers.delete(fileKey);
        files.delete(fileKey);
        app.shutdown((err) => {
          if (err) {
            debug('ignore error on file shutdown: %s', err.message);
          }
        });
      }
    }
  }

  const appender = (logEvent) => {
    const fileKey = findFileKey(config.property, logEvent);
    debug('fileKey for property ', config.property, ' is ', fileKey);
    if (fileKey) {
      let file = files.get(fileKey);
      debug('existing file appender is ', file);
      if (!file) {
        debug('creating new file appender');
        config.filename = path.join(config.base, fileKey + config.extension);
        file = fileAppender.configure(config, layouts);
        files.set(fileKey, file);
        if (config.timeout) {
          debug('creating new timer');
          timers.set(fileKey, {
            timeout: config.timeout,
            lastUsed: Date.now(),
            interval: setInterval(checkForTimeout.bind(null, fileKey), config.timeout)
          });
        }
      } else if (config.timeout) {
        timers.get(fileKey).lastUsed = Date.now();
      }

      file(logEvent);
    } else {
      debug('No fileKey for logEvent, quietly ignoring this log event');
    }
  };

  appender.shutdown = (cb) => {
    let shutdownFunctions = files.size;
    if (shutdownFunctions <= 0) {
      cb();
    }
    let error;
    timers.forEach((timer) => {
      clearInterval(timer.interval);
    });
    files.forEach((app, fileKey) => {
      debug('calling shutdown for ', fileKey);
      app.shutdown((err) => {
        error = error || err;
        shutdownFunctions -= 1;
        if (shutdownFunctions <= 0) {
          cb(error);
        }
      });
    });
  };

  return appender;
};
