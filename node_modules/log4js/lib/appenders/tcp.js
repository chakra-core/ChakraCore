'use strict';

const debug = require('debug')('log4js:tcp');
const net = require('net');

function appender(config) {
  let canWrite = false;
  const buffer = [];
  let socket;
  let shutdownAttempts = 3;

  function write(loggingEvent) {
    debug('Writing log event to socket');
    canWrite = socket.write(`${loggingEvent.serialise()}__LOG4JS__`, 'utf8');
  }

  function emptyBuffer() {
    let evt;
    debug('emptying buffer');
    /* eslint no-cond-assign:0 */
    while ((evt = buffer.shift())) {
      write(evt);
    }
  }

  function createSocket() {
    debug(`appender creating socket to ${config.host || 'localhost'}:${config.port || 5000}`);
    socket = net.createConnection(config.port || 5000, config.host || 'localhost');
    socket.on('connect', () => {
      debug('socket connected');
      emptyBuffer();
      canWrite = true;
    });
    socket.on('drain', () => {
      debug('drain event received, emptying buffer');
      canWrite = true;
      emptyBuffer();
    });
    socket.on('timeout', socket.end.bind(socket));
    // don't bother listening for 'error', 'close' gets called after that anyway
    socket.on('close', createSocket);
  }

  createSocket();

  function log(loggingEvent) {
    if (canWrite) {
      write(loggingEvent);
    } else {
      debug('buffering log event because it cannot write at the moment');
      buffer.push(loggingEvent);
    }
  }

  log.shutdown = function (cb) {
    debug('shutdown called');
    if (buffer.length && shutdownAttempts) {
      debug('buffer has items, waiting 100ms to empty');
      shutdownAttempts -= 1;
      setTimeout(() => {
        log.shutdown(cb);
      }, 100);
    } else {
      socket.removeAllListeners('close');
      socket.end(cb);
    }
  };
  return log;
}

function configure(config) {
  debug(`configure with config = ${config}`);
  return appender(config);
}

module.exports.configure = configure;
