/**
 * read-file <https://github.com/assemble/read-file>
 *
 * Copyright (c) 2014, 2015 Jon Schlinkert.
 * Licensed under the MIT license.
 */

var fs = require('fs');

function read(fp, opts, cb) {
  if (typeof opts === 'function') {
    cb = opts;
    opts = {};
  }

  if (typeof cb !== 'function') {
    throw new TypeError('read-file async expects a callback function.');
  }

  if (typeof fp !== 'string') {
    cb(new TypeError('read-file async expects a string.'));
  }

  fs.readFile(fp, opts, function (err, buffer) {
    if (err) return cb(err);
    cb(null, normalize(buffer, opts));
  });
}

read.sync = function(fp, opts) {
  if (typeof fp !== 'string') {
    throw new TypeError('read-file sync expects a string.');
  }
  try {
    return normalize(fs.readFileSync(fp, opts), opts);
  } catch (err) {
    err.message = 'Failed to read "' + fp + '": ' + err.message;
    throw new Error(err);
  }
};

function normalize(str, opts) {
  str = stripBom(str);
  if (typeof opts === 'object' && opts.normalize === true) {
    return String(str).replace(/\r\n|\n/g, '\n');
  }
  return str;
}

function stripBom(str) {
  return typeof str === 'string' && str.charAt(0) === '\uFEFF'
    ? str.slice(1)
    : str;
}

/**
 * Expose `read`
 */

module.exports = read;