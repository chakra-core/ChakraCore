var arrayUniq = require('array-uniq');

function Charset() {
  this.chars = '';
}

Charset.prototype.setType = function(type) {
  var chars;
  
  var numbers    = '0123456789';
  var charsLower = 'abcdefghijklmnopqrstuvwxyz';
  var charsUpper = charsLower.toUpperCase();
  var hexChars   = 'abcdef';
  
  if (type === 'alphanumeric') {
    chars = numbers + charsLower + charsUpper;
  }
  else if (type === 'numeric') {
    chars = numbers;
  }
  else if (type === 'alphabetic') {
    chars = charsLower + charsUpper;
  }
  else if (type === 'hex') {
    chars = numbers + hexChars;
  }
  else {
    chars = type;
  }
  
  this.chars = chars;
}

Charset.prototype.removeUnreadable = function() {
  var unreadableChars = /[0OIl]/g;
  this.chars = this.chars.replace(unreadableChars, '');
}

Charset.prototype.setcapitalization = function(capitalization) {
  if (capitalization === 'uppercase') {
    this.chars = this.chars.toUpperCase();
  }
  else if (capitalization === 'lowercase') {
    this.chars = this.chars.toLowerCase();
  }
}

Charset.prototype.removeDuplicates = function() {
  var charMap = this.chars.split('');
  charMap = arrayUniq(charMap);
  this.chars = charMap.join('');
}

module.exports = exports = Charset;