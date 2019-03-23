'use strict';

const fs = require('fs-extra');
const path = require('path');
const RSVP = require('rsvp');

const copy = RSVP.denodeify(fs.copy);

let rootPath = process.cwd();

module.exports = function copyFixtureFiles(sourceDir) {
  return copy(path.join(rootPath, 'tests', 'fixtures', sourceDir), '.', { overwrite: true });
};
