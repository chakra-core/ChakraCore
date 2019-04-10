'use strict';

/*
 * Given a full path (no path-resolution is performed), find all the files underneath that
 * path (to the leaves of the directory tree), excluding any node_modules subdirectories.
 * Collect stat information about each file and join the information together in a string,
 * then return that string.
 */
var crypto = require('crypto');
var fs = require('fs');
var path = require('path');

function getFileInfos(fullPath, _visited) {
  var visited = Array.isArray(_visited) ? _visited : [];
  var realPath = fs.realpathSync(fullPath);

  if (visited.indexOf(realPath) >= 0) {
    return [];
  } else {
    visited.push(realPath);
  }

  var stat = fs.statSync(fullPath);

  if (stat.isFile()) {
    return [{
      fullPath: fullPath,
      mtime: stat.mtime.getTime(),
      mode: stat.mode,
      size: stat.size
    }];
  } else if (stat.isDirectory()) {
    // if it ends with node_modules do nothing
    return fs.readdirSync(realPath).sort().reduce(function(paths, entry) {
      if (entry.toLowerCase() === 'node_modules') {
        return paths;
      }

      var keys;
      try {
        keys = getFileInfos(path.join(realPath, entry), visited);
      } catch (err) {
        if (typeof err === "object" && err !== null && err.code !== 'ENOENT') {
          throw err;
        }

        keys = ['missing'];
      }

      return paths.concat(keys);
    }, []);
  } else {
    throw new Error('"' + fullPath + '": Unexpected file type');
  }
}

function stringifyFileInfo(fileInfo) {
  return '|' + fileInfo.mtime + '|' + fileInfo.mode + '|' + fileInfo.size;
}

var hasWorkingFrom = false;
// node 5.x has a incomplete `Buffer.from`
if (typeof Buffer.from === 'function') {
  try {
    Buffer.from([]);
    hasWorkingFrom = true;
  } catch (e) {
    hasWorkingFrom = false;
  }
}

module.exports = function hashTree(fullPath) {
  var strings = getFileInfos(fullPath).sort().map(stringifyFileInfo).join();

  // Once we drop Node < 6 support, we can simplify this code to Buffer.from
  var buf = (hasWorkingFrom  ? Buffer.from(strings) : (new Buffer(strings, 'utf8')));

  return crypto.createHash('md5').update(buf).digest('hex');
};

module.exports.stringifyFileInfo = stringifyFileInfo;
module.exports.getFileInfos = getFileInfos;
