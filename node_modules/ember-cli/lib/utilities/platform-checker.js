'use strict';

const semver = require('semver');
const logger = require('heimdalljs-logger')('ember-cli:platform-checker:');
const loadConfig = require('./load-config');

let testedEngines;
if (process.platform === 'win32') {
  testedEngines = loadConfig('appveyor.yml')
    .environment.matrix
    .map(element => element.nodejs_version)
    .join(' || ');
} else {
  let travisConfig = loadConfig('.travis.yml');
  let nodeVersions = new Set(travisConfig.node_js);

  travisConfig.jobs.include.forEach(job => {
    if (job.node_js) {
      nodeVersions.add(job.node_js);
    }
  });

  testedEngines = Array.from(nodeVersions).join(' || ');
}

let supportedEngines = loadConfig('package.json').engines.node;

module.exports = class PlatformChecker {
  constructor(version) {
    this.version = version;
    this.isValid = this.checkIsValid();
    this.isTested = this.checkIsTested();
    this.isDeprecated = this.checkIsDeprecated();

    logger.info('%o', {
      version: this.version,
      isValid: this.isValid,
      isTested: this.isTested,
      isDeprecated: this.isDeprecated,
    });
  }

  checkIsValid(range) {
    range = range || supportedEngines;
    return semver.satisfies(this.version, range) || semver.gtr(this.version, range);
  }

  checkIsDeprecated(range) {
    return !this.checkIsValid(range);
  }

  checkIsTested(range) {
    range = range || testedEngines;
    return semver.satisfies(this.version, range);
  }
};
