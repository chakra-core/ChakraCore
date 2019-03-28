'use strict';

var isObject = require('isobject');
var isBinary = require('is-binary-buffer');

module.exports = function(file) {
  if (!isObject(file)) {
    throw new Error('expected file to be an object');
  }

  if (file.hasOwnProperty('_isBinary')) {
    return file._isBinary;
  }

  if (typeof file.isStream === 'function' && file.isStream()) {
    file._isBinary = false;
    return false;
  }

  if (typeof file.isNull === 'function' && file.isNull()) {
    file._isBinary = false;
    return false;
  }

  if (typeof file.isDirectory === 'function' && file.isDirectory()) {
    file._isBinary = false;
    return false;
  }

  file._isBinary = isBinary(file.contents);
  return file._isBinary;
};

