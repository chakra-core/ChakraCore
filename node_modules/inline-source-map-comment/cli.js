#!/usr/bin/env node
'use strict';

var argv = require('minimist')(process.argv.slice(2), {
  alias: {
    css: 'block',
    b: 'block',
    c: 'block',
    s: 'sources-content',
    'in': 'input',
    i: 'input',
    h: 'help',
    v: 'version'
  },
  string: ['_', 'input'],
  boolean: ['block', 'sources-content', 'help', 'version']
});

function help() {
  var sumUp = require('sum-up');
  var yellow = require('chalk').yellow;

  var pkg = require('./package.json');

  console.log([
    sumUp(pkg),
    '',
    'Usage1: ' + pkg.name + ' <source map string>',
    'Usage2: ' + pkg.name + ' --in <source map file>',
    'Usage3: cat <source map file> | ' + pkg.name,
    '',
    'Options:',
    yellow('--block, --css,    -b, -c') + '  Print a block comment instead of line comment',
    yellow('--sources-content, -s    ') + '  Preserve sourcesContent property',
    yellow('--in, --input,     -i    ') + '  Use a JSON file as a source',
    yellow('--help,            -h    ') + '  Print usage information',
    yellow('--version,         -v    ') + '  Print version',
    ''
  ].join('\n'));
}

function run(map) {
  if (!map) {
    help();
    return;
  }

  var inlineSourceMapComment = require('./');

  console.log(inlineSourceMapComment(map, {
    block: argv.block,
    sourcesContent: argv['sources-content']
  }));
}

var inputFile = argv.input;

if (argv.version) {
  console.log(require('./package.json').version);
} else if (argv.help) {
  help();
} else if (inputFile) {
  var fs = require('fs');
  try {
    run(fs.readFileSync(inputFile, 'utf8'));
  } catch (e) {
    process.stderr.write(e.stack + '\n', function() {
      process.exit(1);
    });
  }
} else if (process.stdin.isTTY) {
  run(argv._[0]);
} else {
  require('get-stdin')(run);
}
