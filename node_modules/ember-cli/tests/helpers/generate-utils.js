'use strict';

const RSVP = require('rsvp');
const fs = require('fs-extra');
let outputFile = RSVP.denodeify(fs.outputFile);
const ember = require('./ember');

function inRepoAddon(path) {
  return ember([
    'generate',
    'in-repo-addon',
    path,
  ]);
}

function tempBlueprint() {
  return outputFile(
    'blueprints/foo/files/__root__/foos/__name__.js',
    '/* whoah, empty foo! */'
  );
}

module.exports = {
  inRepoAddon,
  tempBlueprint,
};
