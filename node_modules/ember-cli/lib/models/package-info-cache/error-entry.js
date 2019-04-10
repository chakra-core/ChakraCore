'use strict';

/*
 * Small utility class to contain data about a single error found
 * during loading of a package into the PackageInfoCache.
 *
 * @protected
 * @class ErrorEntry
 */
class ErrorEntry {
  constructor(type, data) {
    this.type = type;
    this.data = data;
  }
}

module.exports = ErrorEntry;
