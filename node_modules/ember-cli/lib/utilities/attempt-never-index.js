'use strict';

let isDarwin = (/darwin/i).test(require('os').type());
const logger = require('heimdalljs-logger')('ember-cli:utilities/attempt-metadata-index-file');

/*
 * Writes a `.metadata_never_index` file to the specified folder if running on OS X.
 *
 * This hints to spotlight that this folder should not be indexed.
 *
 * @param {String} dir path to the folder that should not be indexed
 */
module.exports = function(dir) {
  let path = `${dir}/.metadata_never_index`;

  if (!isDarwin) {
    logger.info('not darwin, skipping %s (which hints to spotlight to prevent indexing)', path);
    return;
  }

  logger.info('creating: %s (to prevent spotlight indexing)', path);

  const fs = require('fs-extra');

  fs.mkdirsSync(dir);
  fs.writeFileSync(path);
};
