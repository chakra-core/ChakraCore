'use strict';

const fs = require('fs');
const semver = require('semver');

function getVersionFromJSONFile(filePath) {
  if (fs.existsSync(filePath)) {
    let content = fs.readFileSync(filePath);

    try {
      return JSON.parse(content).version;
    } catch (exception) {
      return null;
    }
  }
}

/**
 * DependencyVersionChecker
 */
class DependencyVersionChecker {
  constructor(parent, name) {
    this._parent = parent;
    this.name = name;
  }

  get version() {
    if (this._version === undefined && this._jsonPath) {
      this._version = getVersionFromJSONFile(this._jsonPath);
    }

    if (this._version === undefined && this._fallbackJsonPath) {
      this._version = getVersionFromJSONFile(this._fallbackJsonPath);
    }

    return this._version;
  }

  exists() {
    return this.version !== undefined;
  }

  isAbove(compareVersion) {
    if (!this.version) {
      return false;
    }
    return semver.gt(this.version, compareVersion);
  }

  assertAbove(compareVersion, _message) {
    let message = _message;

    if (!message) {
      message = `The addon \`${this._parent._addon.name}\` requires the ${
        this._type
      } package \`${this.name}\` to be above ${compareVersion}, but you have ${
        this.version
      }.`;
    }

    if (!this.isAbove(compareVersion)) {
      let error = new Error(message);

      error.suppressStacktrace = true;

      throw error;
    }
  }
}

let semverMethods = ['gt', 'lt', 'gte', 'lte', 'eq', 'neq', 'satisfies'];
semverMethods.forEach(function(method) {
  DependencyVersionChecker.prototype[method] = function(range) {
    if (!this.version) {
      return method === 'neq';
    }
    return semver[method](this.version, range);
  };
});

module.exports = DependencyVersionChecker;
