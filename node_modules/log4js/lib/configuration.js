'use strict';

const util = require('util');
const debug = require('debug')('log4js:configuration');

const listeners = [];

const not = thing => !thing;

const anObject = thing => thing && typeof thing === 'object' && !Array.isArray(thing);

const validIdentifier = thing => /^[A-Za-z][A-Za-z0-9_]*$/g.test(thing);

const anInteger = thing => thing && typeof thing === 'number' && Number.isInteger(thing);

const addListener = (fn) => {
  listeners.push(fn);
  debug(`Added listener, listeners now ${listeners.length}`);
};

const throwExceptionIf = (config, checks, message) => {
  const tests = Array.isArray(checks) ? checks : [checks];
  tests.forEach((test) => {
    if (test) {
      throw new Error(`Problem with log4js configuration: (${util.inspect(config, { depth: 5 })})`
        + ` - ${message}`);
    }
  });
};

const configure = (candidate) => {
  debug('New configuration to be validated: ', candidate);
  throwExceptionIf(candidate, not(anObject(candidate)), 'must be an object.');

  debug(`Calling configuration listeners (${listeners.length})`);
  listeners.forEach(listener => listener(candidate));
  debug('Configuration finished.');
};

module.exports = {
  configure,
  addListener,
  throwExceptionIf,
  anObject,
  anInteger,
  validIdentifier,
  not
};
