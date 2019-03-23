'use strict';

const fs = require('fs-extra');

module.exports = {
  description: 'Generates a lib directory for in-repo addons.',

  normalizeEntityName(name) { return name; },

  beforeInstall() {
    // make sure to create `lib` directory
    fs.mkdirsSync('lib');
  },
};
