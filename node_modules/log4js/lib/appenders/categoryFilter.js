'use strict';

const debug = require('debug')('log4js:categoryFilter');

function categoryFilter(excludes, appender) {
  if (typeof excludes === 'string') excludes = [excludes];
  return (logEvent) => {
    debug(`Checking ${logEvent.categoryName} against ${excludes}`);
    if (excludes.indexOf(logEvent.categoryName) === -1) {
      debug('Not excluded, sending to appender');
      appender(logEvent);
    }
  };
}

function configure(config, layouts, findAppender) {
  const appender = findAppender(config.appender);
  return categoryFilter(config.exclude, appender);
}

module.exports.configure = configure;
