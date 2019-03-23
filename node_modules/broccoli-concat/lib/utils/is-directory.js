'use strict';

/**
 * files returned from listFiles are directories if they end in /
 * see: https://github.com/joliss/node-walk-sync
 * "Note that directories come before their contents, and have a trailing slash"
 *
 * @private
 */
module.exports = function isDirectory(fullPath) {
  return fullPath.charAt(fullPath.length - 1) === '/';
};
