const debug = require('debug')('log4js:clustering');
const LoggingEvent = require('./LoggingEvent');
const configuration = require('./configuration');
const cluster = require('cluster');

const listeners = [];

let disabled = false;
let pm2 = false;
let pm2InstanceVar = 'NODE_APP_INSTANCE';

const isPM2Master = () => pm2 && process.env[pm2InstanceVar] === '0';
const isMaster = () => disabled || cluster.isMaster || isPM2Master();

const sendToListeners = (logEvent) => {
  listeners.forEach(l => l(logEvent));
};

// in a multi-process node environment, worker loggers will use
// process.send
const receiver = (worker, message) => {
  // prior to node v6, the worker parameter was not passed (args were message, handle)
  debug('cluster message received from worker ', worker, ': ', message);
  if (worker.topic && worker.data) {
    message = worker;
    worker = undefined;
  }
  if (message && message.topic && message.topic === 'log4js:message') {
    debug('received message: ', message.data);
    const logEvent = LoggingEvent.deserialise(message.data);
    sendToListeners(logEvent);
  }
};

configuration.addListener((config) => {
  // clear out the listeners, because configure has been called.
  listeners.length = 0;

  disabled = config.disableClustering;
  pm2 = config.pm2;
  pm2InstanceVar = config.pm2InstanceVar || 'NODE_APP_INSTANCE';

  debug(`clustering disabled ? ${disabled}`);
  debug(`cluster.isMaster ? ${cluster.isMaster}`);
  debug(`pm2 enabled ? ${pm2}`);
  debug(`pm2InstanceVar = ${pm2InstanceVar}`);
  debug(`process.env[${pm2InstanceVar}] = ${process.env[pm2InstanceVar]}`);

  // just in case configure is called after shutdown
  if (pm2) {
    process.removeListener('message', receiver);
  }
  if (cluster.removeListener) {
    cluster.removeListener('message', receiver);
  }

  if (config.disableClustering) {
    debug('Not listening for cluster messages, because clustering disabled.');
  } else if (isPM2Master()) {
    // PM2 cluster support
    // PM2 runs everything as workers - install pm2-intercom for this to work.
    // we only want one of the app instances to write logs
    debug('listening for PM2 broadcast messages');
    process.on('message', receiver);
  } else if (cluster.isMaster) {
    debug('listening for cluster messages');
    cluster.on('message', receiver);
  } else {
    debug('not listening for messages, because we are not a master process');
  }
});

module.exports = {
  onlyOnMaster: (fn, notMaster) => (isMaster() ? fn() : notMaster),
  isMaster: isMaster,
  send: (msg) => {
    if (isMaster()) {
      sendToListeners(msg);
    } else {
      if (!pm2) {
        msg.cluster = {
          workerId: cluster.worker.id,
          worker: process.pid
        };
      }
      process.send({ topic: 'log4js:message', data: msg.serialise() });
    }
  },
  onMessage: (listener) => {
    listeners.push(listener);
  }
};
