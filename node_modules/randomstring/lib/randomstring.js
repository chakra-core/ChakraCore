"use strict";

var crypto  = require('crypto');
var Charset = require('./charset.js');

function safeRandomBytes(length) {
  while (true) {
    try {
      return crypto.randomBytes(length);
    } catch(e) {
      continue;
    }
  }
}

exports.generate = function(options) {
  
  var charset = new Charset();
  
  var length, chars, capitalization, string = '';
  
  // Handle options
  if (typeof options === 'object') {
    length = options.length || 32;
    
    if (options.charset) {
      charset.setType(options.charset);
    }
    else {
      charset.setType('alphanumeric');
    }
    
    if (options.capitalization) {
      charset.setcapitalization(options.capitalization);
    }
    
    if (options.readable) {
      charset.removeUnreadable();
    }
    
    charset.removeDuplicates();
  }
  else if (typeof options === 'number') {
    length = options;
    charset.setType('alphanumeric');
  }
  else {
    length = 32;
    charset.setType('alphanumeric');
  }
  
  // Generate the string
  var charsLen = charset.chars.length;
  var maxByte = 256 - (256 % charsLen);
  while (length > 0) {
    var buf = safeRandomBytes(Math.ceil(length * 256 / maxByte));
    for (var i = 0; i < buf.length && length > 0; i++) {
      var randomByte = buf.readUInt8(i);
      if (randomByte < maxByte) {
        string += charset.chars.charAt(randomByte % charsLen);
        length--;
      }
    }
  }

  return string;
};
