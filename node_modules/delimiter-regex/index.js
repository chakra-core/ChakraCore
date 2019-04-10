'use strict';

var util = require('util');
var isObject = require('isobject');
var extend = require('extend-shallow');

module.exports = function delimiterRegex(open, close, options) {
  if (open instanceof RegExp) return open;
  if (typeof open === 'undefined') {
    return /\$\{([^\\}]*(?:\\.[^\\}]*)*)\}/g;
  }
  if (typeof open === 'string' && typeof close === 'string') {
    return createRegex(open, close, options);
  }
  if (Array.isArray(open)) {
    return createRegex(open[0], open[1], close);
  }
  if (isObject(open)) {
    return createRegex(null, null, open);
  }
  var args = [].slice.call(arguments);
  throw new Error('cannot created delimiters from: ' + util.inspect(args));
};

function createRegex(open, close, options) {
  var opts = extend({flags: ''}, options);
  var tagname = opts.tagname ? createTag(opts.tagname) : '([\\s\\S]+?)';
  open = open || opts.open || '\\$\\{';
  close = close || opts.close || '\\}';
  return new RegExp(open + tagname + close, opts.flags);
}

function createTag(str) {
  return ` (${str.trim()}) `;
}
