/*!
 * rethrow <https://github.com/jonschlinkert/rethrow>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var lazy = require('lazy-cache')(require);
lazy('right-align', 'align');
lazy('extend-shallow', 'extend');
lazy('ansi-yellow', 'yellow');
lazy('ansi-bgred', 'bgred');
lazy('ansi-red', 'red');

/**
 * Re-throw the given `err` in context to the offending
 * template expression in `filename` at the given `lineno`.
 *
 * @param {Error} `err` Error object
 * @param {String} `filename` The file path of the template
 * @param {String} `lineno` The line number of the expression causing the error.
 * @param {String} `str` Template string
 * @api public
 */

function rethrow(options) {
  var opts = lazy.extend({before: 3, after: 3}, options);
  return function (err, filename, lineno, str, expr) {
    if (!(err instanceof Error)) throw err;

    lineno = lineno >> 0;
    var lines = str.split('\n');
    var before = Math.max(lineno - (+opts.before), 0);
    var after = Math.min(lines.length, lineno + (+opts.after));

    // align line numbers
    var n = lazy.align(count(lines.length + 1));

    lineno++;
    var pointer = errorMessage(err, opts);

    // Error context
    var context = lines.slice(before, after).map(function (line, i) {
      var num = i + before + 1;
      var msg = style(num, lineno);

      return msg('  > ', '    ')
        + n[num] + '| '
        + line + ' ' + msg(expr, '');
    }).join('\n');

    // Alter exception message
    err.path = filename;
    err.message = (filename || 'source') + ':'
      + lineno + '\n'
      + context + '\n\n'
      + (pointer ? (pointer + lazy.yellow(filename)) : styleMessage(err.message, opts)) + '\n';

    throw err.message;
  };
}

function style(curr, lineno) {
  return function(a, b) {
    return curr == lineno ? lazy.bgred(a || '') : (b || '');
  };
}

function styleMessage(msg, opts) {
  if (typeof opts.styleMessage === 'function') {
    return opts.styleMessage(msg);
  }
  return lazy.red(msg);
}

function count(n) {
  return Array.apply(null, {length: n}).map(Number.call, Number);
}

function errorMessage(err, opts) {
  if (opts.pointer === false) return '';
  var messageRe = /(?:(.+)is not defined|cannot read property)/i, m;
  var str = '';
  if (m = messageRe.exec(err.message)) {
    if (m[1]) {
      var prop = m[1].trim();
      str = 'variable `' + prop + '` is not defined in';
    } else if (err.helper) {
      str = ' helper `' + err.helper.name
        + '` cannot resolve argument `'
        + err.helper.args[0] + '`';
    }
    str = lazy.red('Error: ' + str + ': ');
  }
  return str;
}

/**
 * Expose `rethrow`
 */

module.exports = rethrow;
