#!/usr/bin/env node
'use strict';
const pkg = require('./package.json');
const yosay = require('.');

require('taketalk')({
  init(input, options) {
    console.log(yosay(input, options));
  },
  help() {
    console.log(`
  ${pkg.description}

  Usage
    $ yosay <string>
    $ yosay <string> --maxLength 8
    $ echo <string> | yosay

  Example
    $ yosay 'Sindre is a horse'
    ${yosay('Sindre is a horse')}`);
  },
  version: pkg.version
});
