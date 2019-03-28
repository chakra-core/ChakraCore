'use strict';

var isWhitespace = require('is-whitespace');

module.exports = function(str) {
  if (typeof str !== 'string') {
    throw new TypeError('expected a string');
  }
  if (!str) return str;
  var lines = str.split('\n');
  if (lines.length === 1) {
    return str;
  }
  var start = 0;
  for (var i = 0; i < lines.length; i++) {
    var line = lines[i];
    if (isWhitespace(line) || line === '') {
      continue;
    }
    start = i;
    break;
  }
  return lines.slice(start).join('\n');
};
