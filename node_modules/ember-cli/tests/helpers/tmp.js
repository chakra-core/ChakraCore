'use strict';

const fs = require('fs-extra');
const RSVP = require('rsvp');

let remove = RSVP.denodeify(fs.remove);
let root = process.cwd();

module.exports.setup = function(path) {
  process.chdir(root);

  return remove(path)
    .then(function() {
      fs.mkdirsSync(path);
    });
};

module.exports.teardown = function(path) {
  process.chdir(root);
  return remove(path);
};
