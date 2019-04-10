'use strict';

var Entry = require('./entry');

function validateSortedUnique(entries) {
  for (var i = 1; i < entries.length; i++) {
    var previous = entries[i - 1].relativePath;
    var current = entries[i].relativePath;

    if (previous < current) {
      continue;
    } else {
      throw new Error('expected entries[' + (i -1) + ']: `' + previous +
                      '` to be < entries[' + i + ']: `' + current + '`, but was not. Ensure your input is sorted and has no duplicate paths');
    }
  }
}


function commonPrefix(a, b, term) {
  var max = Math.min(a.length, b.length);
  var end = -1;

  for(var i = 0; i < max; ++i) {
    if (a[i] !== b[i]) {
      break;
    } else if (a[i] === term) {
      end = i;
    }
  }

  return a.substr(0, end + 1);
}

function basename(entry) {
  var path = entry.relativePath;
  var end = path.length - 2;
  for(var i = end; i >= 0; --i) {
    if (path[i] === '/') {
      return path.substr(0, i + 1);
    }
  }

  return '';
}

function computeImpliedEntries(basePath, relativePath) {
  var rv = [];

  for (var i=0; i<relativePath.length; ++i) {
    if (relativePath[i] === '/') {
      var path = basePath + relativePath.substr(0, i + 1);
      rv.push(new Entry(path, 0, 0));
    }
  }

  return rv;
}

function compareByRelativePath(entryA, entryB) {
  var pathA = entryA.relativePath;
  var pathB = entryB.relativePath;

  if (pathA < pathB) {
    return -1;
  } else if (pathA > pathB) {
    return 1;
  }

  return 0;
}

function sortAndExpand(entries) {
  entries.sort(compareByRelativePath);

  var path = '';

  for (var i=0; i<entries.length; ++i) {
    var entry = entries[i];

    // update our path eg
    //    path = a/b/c/d/
    //    entry = a/b/q/r/s/
    //    path' = a/b/
    path = commonPrefix(path, entry.relativePath, '/');

    // a/b/ -> a/
    // a/b  -> a/
    var base = basename(entry);
    // base - path
    var entryBaseSansCommon = base.substr(path.length);
    // determine what intermediate directories are missing eg
    //    path = a/b/
    //    entryBaseSansCommon = c/d/e/
    //    impliedEntries = [a/b/c/, a/b/c/d/, a/b/c/d/e/]
    var impliedEntries = computeImpliedEntries(path, entryBaseSansCommon);

    // actually add our implied entries to entries
    if (impliedEntries.length > 0) {
      entries.splice.apply(entries, [i, 0].concat(impliedEntries));
      i += impliedEntries.length;
    }

    // update path.  Now that we've created all the intermediate directories, we
    // don't need to recreate them for subsequent entries.
    if (entry.isDirectory()) {
      path = entry.relativePath;
    } else {
      path = base;
    }
  }

  return entries;
}

module.exports = {
  validateSortedUnique: validateSortedUnique,
  sortAndExpand: sortAndExpand,

  // exported for testing
  _commonPrefix: commonPrefix,
  _basename: basename,
  _computeImpliedEntries: computeImpliedEntries,
};
