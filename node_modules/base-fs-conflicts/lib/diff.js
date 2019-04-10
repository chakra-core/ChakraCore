'use strict';

var utils = require('./utils');

/**
 * Shows a color-based diff of two strings
 *
 * @param {string} actual
 * @param {string} expected
 */

module.exports = function(actual, expected) {
  var colors = { added: green, removed: utils.colors.bgred };
  var str = utils.diff.diffLines(actual, expected).map(function(obj) {
    if (obj.added) {
      return colorLines(colors, 'added', obj.value);
    }
    if (obj.removed) {
      return colorLines(colors, 'removed', obj.value);
    }
    return obj.value;
  }).join('');

  // display color legend
  var msg = '\n';
  msg += colors.removed('removed') + ' ';
  msg += colors.added('added');
  msg += '\n\n' + str + '\n';
  return msg;
};

function green(msg) {
  return utils.colors.black(utils.colors.bggreen(msg));
}

function colorLines(colors, name, str) {
  return str.split('\n').map(function(str) {
    return colors[name](str);
  }).join('\n');
}
