'use strict';

const os = require('os');

module.exports = function summarizeProcess(process) {
  let PATH = [];
  if (process.env && process.env.PATH) {
    PATH = process.env.PATH.split(/:/);
  }

  return`
  TIME: ${new Date()}
  TITLE: ${process.title || ''}
  ARGV:${arr(process.argv || [])}
  EXEC_PATH: ${process.execPath || ''}
  TMPDIR: ${os.tmpdir()}
  SHELL: ${os.userInfo().shell}
  PATH:${arr(PATH)}
  PLATFORM: ${process.platform || ''} ${process.arch || ''}
  FREEMEM: ${os.freemem()}
  TOTALMEM: ${os.totalmem()}
  UPTIME: ${os.uptime()}
  LOADAVG: ${os.loadavg()}
  CPUS:${arr(os.cpus().map(cpu => '' + cpu.model + ' - ' + cpu.speed))}
  ENDIANNESS: ${os.endianness()}
  VERSIONS:${obj(process.versions)}
`;
}

module.exports.obj = obj;
function obj(o) {
  if (o === undefined) { return ''; }
  let depth;

  if (arguments.length < 2) {
    depth = 0;
  } else {
    depth = arguments[1];
  }

  let indent = '  ';
  for (let i = 0; i < depth; i++){
    indent += '  ';
  }

  let result = Object.keys(o).sort().reduce((acc, key) => {
    let value = o[key];

    if (value === undefined) {
      acc += `\n${indent}- ${key}: [undefined]`
      return acc;
    } else if (value === null) {
      acc += `\n${indent}- ${key}: [null]`
      return acc;
    }

    if (Array.isArray(value)) {
      acc += `\n${indent}- ${key}:`;
      acc += `${arr(value, depth + 1)}`
    } else if (typeof value === 'object') {
      acc += `\n${indent}- ${key}:`
      acc += obj(value, depth + 1);
    } else {
      acc += `\n${indent}- ${key}: ${value}`;
    }

    return acc;
  }, '');

  if (result === '') {
      return `\n${indent}- { }`;
  }
  return result;
}

module.exports.arr = arr;
function arr(array) {
  let depth = arguments.length < 2 ? '  ' : arguments[1];

  let indent = '  ';
  for (let i = 0; i < depth; i++){
    indent += '  ';
  }
  return array.reduce((acc, entry) => {
    if (typeof entry === 'object') {
      if (entry === null) {
        acc += `\n${indent}- [null]`;
      } else {
        acc += `${obj(entry, depth )}`;
      }
    } else {
      acc += `\n${indent}- ${entry}`
    }
    return acc;
  }, '');
}

