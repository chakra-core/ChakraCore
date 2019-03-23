'use strict';

const streams = require('streamroller');
const os = require('os');

const eol = os.EOL || '\n';

/**
 * File appender that rolls files according to a date pattern.
 * @filename base filename.
 * @pattern the format that will be added to the end of filename when rolling,
 *          also used to check when to roll files - defaults to '.yyyy-MM-dd'
 * @layout layout function for log messages - defaults to basicLayout
 * @timezoneOffset optional timezone offset in minutes - defaults to system local
 */
function appender(
  filename,
  pattern,
  layout,
  options,
  timezoneOffset
) {
  const logFile = new streams.DateRollingFileStream(
    filename,
    pattern,
    options
  );

  const app = function (logEvent) {
    logFile.write(layout(logEvent, timezoneOffset) + eol, 'utf8');
  };

  app.shutdown = function (complete) {
    logFile.write('', 'utf-8', () => {
      logFile.end(complete);
    });
  };

  return app;
}

function configure(config, layouts) {
  let layout = layouts.basicLayout;

  if (config.layout) {
    layout = layouts.layout(config.layout.type, config.layout);
  }

  if (!config.alwaysIncludePattern) {
    config.alwaysIncludePattern = false;
  }

  return appender(
    config.filename,
    config.pattern,
    layout,
    config,
    config.timezoneOffset
  );
}

module.exports.configure = configure;
