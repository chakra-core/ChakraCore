'use strict';


// eslint-disable-next-line node/no-unpublished-require
const expect = require('chai').expect;
const semver = require('semver');

module.exports = function assertVersionLock(_deps) {
  let deps = _deps || {};

  Object.keys(deps).forEach(function(name) {
    if (name !== 'ember-cli' &&
        semver.valid(deps[name]) &&
        semver.gtr('1.0.0', deps[name])) {
      // only valid if the version is fixed
      expect(semver.valid(deps[name]), `"${name}" has a valid version`).to.be.ok;
    }
  });
};
