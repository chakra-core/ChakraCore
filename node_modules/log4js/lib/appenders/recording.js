'use strict';

const debug = require('debug')('log4js:recording');

const recordedEvents = [];

function configure() {
  return function (logEvent) {
    debug(`received logEvent, number of events now ${recordedEvents.length + 1}`);
    debug('log event was ', logEvent);
    recordedEvents.push(logEvent);
  };
}

function replay() {
  return recordedEvents.slice();
}

function reset() {
  recordedEvents.length = 0;
}

module.exports = {
  configure: configure,
  replay: replay,
  playback: replay,
  reset: reset,
  erase: reset
};
