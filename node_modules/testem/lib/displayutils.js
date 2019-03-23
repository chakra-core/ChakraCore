'use strict';

const util = require('util');

// Method to format test results.
const strutils = require('./strutils');

function resultDisplay(id, prefix, result) {
  let parts = [];

  if (prefix) {
    parts.push(prefix);
  }

  parts.push(`[${result.runDuration} ms]`);

  if (result.name) {
    parts.push(result.name.trim());
  }

  let line = parts.join(' - ');
  let output = (result.skipped ? 'skip ' : (result.passed ? 'ok ' : 'not ok ')) + id + ' ' + line;

  return output;
}

function yamlDisplay(err, logs) {
  let testLogs;
  let failed = Object.keys(err || {})
    .filter(key => key !== 'passed')
    .map(key => key + ': >\n' + strutils.indent(String(err[key])));
  if (logs) {
    testLogs = ['Log: |'].concat(logs.map(log => strutils.indent(util.inspect(log))));
  } else {
    testLogs = [];
  }
  return strutils.indent([
    '---',
    strutils.indent(failed.concat(testLogs).join('\n')),
    '...'].join('\n'));
}

function resultString(id, prefix, result, quietLogs) {
  let string = resultDisplay(id, prefix, result) + '\n';
  if (result.error || (!quietLogs && result.logs && result.logs.length)) {
    string += yamlDisplay(result.error, result.logs) + '\n';
  }
  return string;
}

exports.resultString = resultString;
