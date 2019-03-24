'use strict';

var fs = require('fs');
var MatcherCollection = require('matcher-collection');
var ensurePosix = require('ensure-posix-path');
var path = require('path');

function handleOptions(_options) {
  var options = {};
  if (Array.isArray(_options)) {
    options.globs = _options;
  } else if (_options) {
    options = _options;
  }

  return options;
}

function handleRelativePath(_relativePath) {
  if (_relativePath == null) {
    return '';
  } else if (_relativePath.slice(-1) !== '/') {
    return _relativePath + '/';
  } else {
    return _relativePath;
  }
}

module.exports = walkSync;
function walkSync(baseDir, _options) {
  var options = handleOptions(_options);

  var mapFunct;
  if (options.includeBasePath) {
    mapFunct = function (entry) {
      return entry.basePath.split(path.sep).join('/') + '/' + entry.relativePath;
    };
  } else {
    mapFunct = function (entry) {
        return entry.relativePath;
    };
  }

  return _walkSync(baseDir, options).map(mapFunct);
}

module.exports.entries = function entries(baseDir, _options) {
  var options = handleOptions(_options);

  return _walkSync(ensurePosix(baseDir), options);
};

function _walkSync(baseDir, options, _relativePath) {
  // Inside this function, prefer string concatenation to the slower path.join
  // https://github.com/joyent/node/pull/6929
  var relativePath = handleRelativePath(_relativePath);
  var globs = options.globs;
  var ignorePatterns = options.ignore;
  var globMatcher, ignoreMatcher;
  var results = [];

  if (ignorePatterns) {
    ignoreMatcher = new MatcherCollection(ignorePatterns);
  }

  if (globs) {
    globMatcher = new MatcherCollection(globs);
  }

  if (globMatcher && !globMatcher.mayContain(relativePath)) {
    return results;
  }

  var names = fs.readdirSync(baseDir + '/' + relativePath);
  var entries = names.map(function (name) {
    var entryRelativePath = relativePath + name;

    if (ignoreMatcher && ignoreMatcher.match(entryRelativePath)) {
      return;
    }

    var fullPath = baseDir + '/' + entryRelativePath;
    var stats = getStat(fullPath);

    if (stats && stats.isDirectory()) {
      return new Entry(entryRelativePath + '/', baseDir, stats.mode, stats.size, stats.mtime.getTime());
    } else {
      return new Entry(entryRelativePath, baseDir, stats && stats.mode, stats && stats.size, stats && stats.mtime.getTime());
    }
  }).filter(Boolean);

  var sortedEntries = entries.sort(function (a, b) {
    var aPath = a.relativePath;
    var bPath = b.relativePath;

    if (aPath === bPath) {
      return 0;
    } else if (aPath < bPath) {
      return -1;
    } else {
      return 1;
    }
  });

  for (var i=0; i<sortedEntries.length; ++i) {
    var entry = sortedEntries[i];

    if (entry.isDirectory()) {
      if (options.directories !== false && (!globMatcher || globMatcher.match(entry.relativePath))) {
        results.push(entry);
      }
      results = results.concat(_walkSync(baseDir, options, entry.relativePath));
    } else {
      if (!globMatcher || globMatcher.match(entry.relativePath)) {
        results.push(entry);
      }
    }
  }

  return results;
}

function Entry(relativePath, basePath, mode, size, mtime) {
  this.relativePath = relativePath;
  this.basePath = basePath;
  this.mode = mode;
  this.size = size;
  this.mtime = mtime;
}

Object.defineProperty(Entry.prototype, 'fullPath', {
  get: function() {
    return this.basePath + '/' + this.relativePath;
  }
});

Entry.prototype.isDirectory = function () {
  return (this.mode & 61440) === 16384;
};

function getStat(path) {
  var stat;

  try {
    stat = fs.statSync(path);
  } catch(error) {
    if (error.code !== 'ENOENT') {
      throw error;
    }
  }

  return stat;
}
