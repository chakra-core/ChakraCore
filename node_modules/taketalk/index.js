'use strict';
var argv = require('minimist')(process.argv.slice(2));
var stdin = require('get-stdin');
var input = argv._[0];

var taketalk = {
  init: function () {},
  printHelp: function () { this._doWhatItIs('help'); },
  printVersion: function () { this._doWhatItIs('version'); },
  _doWhatItIs: function (key) {
    var thing = this[key];
    switch (typeof thing) {
      case 'string': console.log(thing); break;
      case 'function': thing(); break;
    }
  }
};

module.exports = function (obj) {
  if ({}.toString.call(obj).indexOf('Object') === -1) {
    throw new Error('Expected an object.');
  }

  for (var prop in obj) {
    taketalk[prop] = obj[prop];
  }

  if (argv.help || argv.h) return taketalk.printHelp();
  if (argv.version || argv.v) return taketalk.printVersion();

  if (process.stdin.isTTY) {
    taketalk.init.call(taketalk, input, argv);
  } else {
    stdin(function (data) {
      taketalk.init.call(taketalk, data, argv);
    });
  }

  return taketalk;
};
