/*!
 * write-json <https://github.com/jonschlinkert/write-json>
 *
 * Copyright (c) 2014-2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var writeFile = require('write');

/**
 * Expose `writeJson`
 */

module.exports = function writeJson(dest, obj, cb) {
  writeFile(dest, JSON.stringify(obj, null, 2), cb);
};

/**
 * Expose `writeJson.sync`
 */

module.exports.sync = function writeJsonSync(dest, obj) {
  try {
    writeFile.sync(dest, JSON.stringify(obj, null, 2));
  } catch (err) {
    throw err;
  }
};
