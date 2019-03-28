'use strict';

var fs = require('fs');
var path = require('path');
var diff = require('./diff');
var utils = require('./utils');

module.exports = function(file, next) {
  var fp = path.resolve(file.dest, file.relative);
  if (isBinary(fp, file.contents)) {
    console.error(binaryDiff(fp, file.contents));
  } else {
    var str = fs.readFileSync(fp, 'utf8');
    console.error(diff(str, file.contents.toString()));
  }
  next();
};

function isBinary(existingPath, fileContents) {
  var header = utils.readChunk.sync(existingPath, 0, 512);
  return utils.itob.isBinarySync(undefined, header)
    || utils.itob.isBinarySync(undefined, fileContents);
}

function binaryDiff(existingPath, fileContents) {
  var existingStat = fs.statSync(existingPath);
  var sizeDiff;

  var table = new utils.Table({
    head: ['', 'Existing', 'Replacement', 'Diff']
  });

  if (existingStat.size > fileContents.length) {
    sizeDiff = '-';
  } else {
    sizeDiff = '+';
  }

  sizeDiff += utils.bytes(Math.abs(existingStat.size - fileContents.length));

  table.push([
    'Size',
    utils.bytes(existingStat.size),
    utils.bytes(fileContents.length),
    sizeDiff
  ], [
    'Last modified',
    utils.dateFormat(existingStat.mtime),
    '',
    ''
  ]);

  return table.toString();
};
