'use strict';

/**
 * A simple class to store metadata info about an addon. This replaces a plain JS object
 * that used to be created with the same fields.
 *
 * @module ember-cli
 */

class AddonInfo {
  constructor(name, path, pkg) {
    this.name = name;
    this.path = path;
    this.pkg = pkg;
  }
}

module.exports = AddonInfo;
