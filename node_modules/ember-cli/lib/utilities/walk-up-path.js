'use strict';

const path = require('path');

let regex = /^[./]$/;

function walkUp(thePath) {
  let paths = [];

  let currentPath = thePath;
  while (true) { // eslint-disable-line no-constant-condition
    currentPath = path.dirname(currentPath);
    if (regex.test(currentPath)) {
      break;
    }
    paths.push(currentPath);
  }

  return paths;
}

module.exports = walkUp;
