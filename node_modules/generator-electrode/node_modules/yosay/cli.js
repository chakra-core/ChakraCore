#!/usr/bin/env node
'use strict';
var pkg = require('./package.json');
var yosay = require('./');

require('taketalk')({
  init: function (input, options) {
    console.log(yosay(input, options));
  },
  help: function () {
    console.log([
      '',
      '  ' + pkg.description,
      '',
      '  Usage',
      '    yosay <string>',
      '    yosay <string> --maxLength 8',
      '    echo <string> | yosay',
      '',
      '  Example',
      '    yosay "Sindre is a horse"',
      yosay('Sindre is a horse')
    ].join('\n'));
  },
  version: pkg.version
});
