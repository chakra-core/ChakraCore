'use strict';

const execa = require('execa');
const RSVP = require('rsvp');
const logger = require('heimdalljs-logger')('ember-cli:execa');

function _execa(cmd, args, opts) {
  logger.info('execa(%j, %j)', cmd, args);
  return RSVP.resolve(execa(cmd, args, opts)).then(result => {
    logger.info('execa(%j, %j) -> code: %d', cmd, args, result.code);
    return result;
  });
}

_execa.sync = function(cmd, args, opts) {
  logger.info('execa.sync(%j, %j)', cmd, args);
  let result = execa.sync(cmd, args, opts);
  logger.info('execa.sync(%j, %j) -> code: %d', cmd, args, result.code);
  return result;
};

module.exports = _execa;
