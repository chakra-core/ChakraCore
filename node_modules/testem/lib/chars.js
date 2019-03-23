'use strict';

const isWin = require('./utils/is-win')();

// Special characters to use for drawing.

if (isWin) {
  // Windows (by default) doesn't support the cool unicode characters
  module.exports = {
    horizontal: '-',
    vertical: '|',
    topLeft: '+',
    topRight: '+',
    bottomLeft: '+',
    bottomRight: '+',
    fail: 'x',
    success: 'v',
    cross: 'x',
    spinner: '-\\|/',
    dot: '.',
    mark: 'x'
  };
} else {
  module.exports = {
    horizontal: '\u2501',
    vertical: '\u2503',
    topLeft: '\u250f',
    topRight: '\u2513',
    bottomLeft: '\u2517',
    bottomRight: '\u251b',
    fail: '\u2718',
    success: '\u2714',
    cross: '\u2718',
    spinner: '\u25dc\u25dd\u25de\u25df',
    dot: '\u00b7',
    mark: '\u2714'
  };
}

