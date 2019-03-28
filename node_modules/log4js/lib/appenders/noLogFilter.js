'use strict';

const debug = require('debug')('log4js:noLogFilter');

/**
 * The function removes empty or null regexp from the array
 * @param {Array<string>} regexp
 * @returns {Array<string>} a filtered string array with not empty or null regexp
 */
function removeNullOrEmptyRegexp(regexp) {
  const filtered = regexp.filter(el => ((el != null) && (el !== '')));
  return filtered;
}

/**
 * Returns a function that will exclude the events in case they match
 * with the regular expressions provided
 * @param {string | Array<string>} filters contains the regexp that will be used for the evaluation
 * @param {*} appender
 * @returns {function}
 */
function noLogFilter(filters, appender) {
  return (logEvent) => {
    debug(`Checking data: ${logEvent.data} against filters: ${filters}`);
    if (typeof filters === 'string') {
      filters = [filters];
    }
    filters = removeNullOrEmptyRegexp(filters);
    const regex = new RegExp(filters.join('|'), 'i');
    if (filters.length === 0 ||
      logEvent.data.findIndex(value => regex.test(value)) < 0) {
      debug('Not excluded, sending to appender');
      appender(logEvent);
    }
  };
}

function configure(config, layouts, findAppender) {
  const appender = findAppender(config.appender);
  return noLogFilter(config.exclude, appender);
}

module.exports.configure = configure;
