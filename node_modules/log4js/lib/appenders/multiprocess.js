'use strict';

const debug = require('debug')('log4js:multiprocess');
const net = require('net');
const LoggingEvent = require('../LoggingEvent');

const END_MSG = '__LOG4JS__';

/**
 * Creates a server, listening on config.loggerPort, config.loggerHost.
 * Output goes to config.actualAppender (config.appender is used to
 * set up that appender).
 */
function logServer(config, actualAppender, levels) {
  /**
   * Takes a utf-8 string, returns an object with
   * the correct log properties.
   */
  function deserializeLoggingEvent(clientSocket, msg) {
    debug('deserialising log event');
    const loggingEvent = LoggingEvent.deserialise(msg);
    loggingEvent.remoteAddress = clientSocket.remoteAddress;
    loggingEvent.remotePort = clientSocket.remotePort;

    return loggingEvent;
  }

  /* eslint prefer-arrow-callback:0 */
  const server = net.createServer(function serverCreated(clientSocket) {
    clientSocket.setEncoding('utf8');
    let logMessage = '';

    function logTheMessage(msg) {
      if (logMessage.length > 0) {
        debug('deserialising log event and sending to actual appender');
        actualAppender(deserializeLoggingEvent(clientSocket, msg));
      }
    }

    function chunkReceived(chunk) {
      debug('chunk of data received');
      let event;
      logMessage += chunk || '';
      if (logMessage.indexOf(END_MSG) > -1) {
        event = logMessage.substring(0, logMessage.indexOf(END_MSG));
        logTheMessage(event);
        logMessage = logMessage.substring(event.length + END_MSG.length) || '';
        // check for more, maybe it was a big chunk
        chunkReceived();
      }
    }

    function handleError(error) {
      const loggingEvent = {
        startTime: new Date(),
        categoryName: 'log4js',
        level: levels.ERROR,
        data: ['A worker log process hung up unexpectedly', error],
        remoteAddress: clientSocket.remoteAddress,
        remotePort: clientSocket.remotePort
      };
      actualAppender(loggingEvent);
    }

    clientSocket.on('data', chunkReceived);
    clientSocket.on('end', chunkReceived);
    clientSocket.on('error', handleError);
  });

  server.listen(config.loggerPort || 5000, config.loggerHost || 'localhost', function () {
    debug('master server listening');
    // allow the process to exit, if this is the only socket active
    server.unref();
  });

  function app(event) {
    debug('log event sent directly to actual appender (local event)');
    return actualAppender(event);
  }

  app.shutdown = function (cb) {
    debug('master shutdown called, closing server');
    server.close(cb);
  };

  return app;
}

function workerAppender(config) {
  let canWrite = false;
  const buffer = [];
  let socket;
  let shutdownAttempts = 3;

  function write(loggingEvent) {
    debug('Writing log event to socket');
    socket.write(loggingEvent.serialise(), 'utf8');
    socket.write(END_MSG, 'utf8');
  }

  function emptyBuffer() {
    let evt;
    debug('emptying worker buffer');
    /* eslint no-cond-assign:0 */
    while ((evt = buffer.shift())) {
      write(evt);
    }
  }

  function createSocket() {
    debug(`worker appender creating socket to ${config.loggerHost || 'localhost'}:${config.loggerPort || 5000}`);
    socket = net.createConnection(config.loggerPort || 5000, config.loggerHost || 'localhost');
    socket.on('connect', () => {
      debug('worker socket connected');
      emptyBuffer();
      canWrite = true;
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
      debug('worker buffering log event because it cannot write at the moment');
      buffer.push(loggingEvent);
    }
  }
  log.shutdown = function (cb) {
    debug('worker shutdown called');
    if (buffer.length && shutdownAttempts) {
      debug('worker buffer has items, waiting 100ms to empty');
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

function createAppender(config, appender, levels) {
  if (config.mode === 'master') {
    debug('Creating master appender');
    return logServer(config, appender, levels);
  }

  debug('Creating worker appender');
  return workerAppender(config);
}

function configure(config, layouts, findAppender, levels) {
  let appender;
  debug(`configure with mode = ${config.mode}`);
  if (config.mode === 'master') {
    if (!config.appender) {
      debug(`no appender found in config ${config}`);
      throw new Error('multiprocess master must have an "appender" defined');
    }
    debug(`actual appender is ${config.appender}`);
    appender = findAppender(config.appender);
    if (!appender) {
      debug(`actual appender "${config.appender}" not found`);
      throw new Error(`multiprocess master appender "${config.appender}" not defined`);
    }
  }
  return createAppender(config, appender, levels);
}

module.exports.configure = configure;
