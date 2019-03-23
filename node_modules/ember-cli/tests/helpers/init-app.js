'use strict';

const ember = require('./ember');

function initApp() {
  return ember([
    'init',
    '--name=my-app',
    '--skip-npm',
    '--skip-bower',
  ]);
}

module.exports = initApp;
