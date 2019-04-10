'use strict';

var check = require('success-symbol');
var green = require('ansi-green');

module.exports = function() {
  var args = [].slice.call(arguments);
  var msg = args[0];
  var ok = green(check);

  if (typeof msg === 'string') {
    var match = /^(\s+)(.*)/.exec(msg);
    if (match) {
      ok = match[1] + ok;
      args[0] = match[2];
    }
  }

  console.log.bind(console, ok).apply(console, args);
};
