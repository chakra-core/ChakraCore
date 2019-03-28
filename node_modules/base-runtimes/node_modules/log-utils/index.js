/*!
 * log-utils <https://github.com/jonschlinkert/log-utils>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var readline = require('readline');
var log = require('ansi-colors');
var fn = require;
require = log;

/**
 * Utils
 */

require('error-symbol');
require('info-symbol');
require('success-symbol', 'check');
require('warning-symbol');
require('time-stamp');
require = fn;

/**
 * Expose `colors` and `symbols`
 */

log.colors = require('ansi-colors');
log.symbol = {};

/**
 * Error symbol.
 *
 * ```js
 * console.log(log.symbol.error);
 * //=> ✖
 * ```
 * @name .symbol.error
 * @api public
 */

getter(log.symbol, 'error', function() {
  return log.errorSymbol;
});

/**
 * Info symbol.
 *
 * ```js
 * console.log(log.symbol.info);
 * //=> ℹ
 * ```
 * @name .symbol.info
 * @api public
 */

getter(log.symbol, 'info', function() {
  return log.infoSymbol;
});

/**
 * Success symbol.
 *
 * ```js
 * console.log(log.symbol.success);
 * //=> ✔
 * ```
 * @name .symbol.success
 * @api public
 */

getter(log.symbol, 'success', function() {
  return log.check;
});

/**
 * Warning symbol.
 *
 * ```js
 * console.log(log.symbol.warning);
 * //=> ⚠
 * ```
 * @name .symbol.warning
 * @api public
 */

getter(log.symbol, 'warning', function() {
  return log.warningSymbol;
});

/**
 * Get a red error symbol.
 *
 * ```js
 * console.log(log.error);
 * //=> ✖
 * ```
 * @name .error
 * @api public
 */

getter(log, 'error', function() {
  return log.red(log.symbol.error);
});

/**
 * Get a cyan info symbol.
 *
 * ```js
 * console.log(log.info);
 * //=> ℹ
 * ```
 * @name .info
 * @api public
 */

getter(log, 'info', function() {
  return log.cyan(log.symbol.info);
});

/**
 * Get a green success symbol.
 *
 * ```js
 * console.log(log.success);
 * //=> ✔
 * ```
 * @name .success
 * @api public
 */

getter(log, 'success', function() {
  return log.green(log.symbol.success);
});

/**
 * Get a yellow warning symbol.
 *
 * ```js
 * console.log(log.warning);
 * //=> ⚠
 * ```
 * @name .warning
 * @api public
 */

getter(log, 'warning', function() {
  return log.yellow(log.symbol.warning);
});

/**
 * Get a formatted timestamp.
 *
 * ```js
 * console.log(log.timestamp);
 * //=> [15:27:46]
 * ```
 * @name .timestamp
 * @api public
 */

getter(log, 'timestamp', function() {
  return '[' + log.gray(log.timeStamp('HH:mm:ss')) + ']';
});

/**
 * Log a white success message prefixed by a green check.
 *
 * ```js
 * log.ok('Alright!');
 * //=> '✔ Alright!'
 * ```
 * @name .ok
 * @api public
 */

log.ok = require('log-ok');

/**
 * Make the given text bold and underlined.
 *
 * ```js
 * console.log(log.heading('foo'));
 * // or
 * console.log(log.heading('foo', 'bar'));
 * ```
 * @name .heading
 * @api public
 */

log.heading = function() {
  var args = [].concat.apply([], [].slice.call(arguments));
  var len = args.length;
  var idx = -1;
  var headings = [];
  while (++idx < len) {
    var str = args[idx];
    if (typeof str === 'string') {
      headings.push(log.bold(log.underline(str)));
    }
  }
  return headings.join(' ');
};

/**
 * Start a basic terminal spinner. Currently this only allows for one
 * spinner at a time, but there are plans to allow multiple named spinners.
 *
 * ```js
 * log.spinner('downloading...');
 * ```
 * @name .spinner
 * @api public
 */

log.spinner = spinner;

function spinner(name, msg, options) {
  options = options || {};
  var interval = options.hasOwnProperty('interval') ? options.interval : 150;
  var chars = options.spinner || ['|', '/', '-', '\\', '-'];
  var len = chars.length;
  var idx = 0;

  spinner.timer = setInterval(function() {
    readline.clearLine();
    readline.cursorTo(1);
    process.stdout.write('\u001b[0G ' + chars[idx++ % len] + ' ' + msg);
  }, interval);
};

/**
 * Stop a spinner.
 *
 * ```js
 * log.spinner.stop('finished downloading');
 * ```
 * @name .spinner.stop
 * @api public
 */

spinner.stop = function stopSpinner(msg) {
  readline.clearLine();
  readline.cursorTo(1);
  process.stdout.write('\u001b[2K' + msg);
  clearInterval(spinner.timer);
};

/**
 * Start a timer with a spinner. Requires an instance of [time-diff][] as
 * the first argument.
 *
 * ```js
 * var Time = require('time-diff');
 * var time = new Time();
 * log.spinner.startTimer(time, 'foo', 'downloading');
 * ```
 * @name .spinner.startTimer
 * @api public
 */

spinner.startTimer = function start(time, name, msg, options) {
  options = options || {};
  if (options.verbose !== false) {
    time.start(name);
    spinner(msg, options);
  }
};

/**
 * Stop a spinner-timer.
 *
 * ```js
 * log.spinner.stopTimer(time, 'foo', 'finished downloading');
 * ```
 * @name .spinner.stopTimer
 * @api public
 */

spinner.stopTimer = function stop(time, name, msg, options) {
  options = options || {};
  if (options.verbose !== false) {
    var ts = log.colors.magenta('+' + time.end(name));
    spinner.stop(' ' + log.success + ' ' + msg + ' ' + ts + '\n');
  }
};

/**
 * Utility for defining a getter
 */

function getter(obj, prop, fn) {
  Object.defineProperty(obj, prop, {
    configurable: true,
    enumerable: true,
    get: fn
  });
}

/**
 * Expose `log`
 */

module.exports = log;

