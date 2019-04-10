'use strict';

const fs = require('fs-extra');
const temp = require('temp');
const RSVP = require('rsvp');

const mkdirTemp = RSVP.denodeify(temp.mkdir);

function mkTmpDirIn(dir) {
  return fs.ensureDir(dir).then(() => mkdirTemp({ dir }));
}

module.exports = mkTmpDirIn;
