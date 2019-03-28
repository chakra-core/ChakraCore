/*!
 * load-pkg <https://github.com/jonschlinkert/load-pkg>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var fs = require('fs');
var findPkg = require('find-pkg');

module.exports = function(dir, cb) {
  if (typeof dir === 'function') {
    cb = dir;
    dir = null;
  }

  if (typeof cb !== 'function') {
    throw new TypeError('load-pkg async expects a callback function');
  }

  findPkg(dir || process.cwd(), function(err, fp) {
    if (err) return cb(err);

    fs.readFile(fp, 'utf8', function(err, str) {
      if (err) return cb(err);
      cb(null, JSON.parse(str));
    });
  });
};

module.exports.sync = function(dir) {
  try {
    var filepath = findPkg.sync(dir || process.cwd());
    var str = fs.readFileSync(filepath, 'utf8');
    return JSON.parse(str);
  } catch (err) {}
  return null;
};
