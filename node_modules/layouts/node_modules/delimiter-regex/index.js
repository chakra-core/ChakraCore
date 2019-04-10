'use strict';

var extend = require('extend-shallow');

module.exports = function delimiters(open, close, options) {
  if (open instanceof RegExp) {
    return open;
  }

  if (typeof close !== 'string') {
    options = close;
    close = null;
  }

  if (typeof open === 'object' && !Array.isArray(open)) {
    options = open;
    open = null;
    close = null;
  }

  if (Array.isArray(open)) {
    var syntax = open.slice();
    open = syntax[0];
    close = syntax[1];
  }

  var opts = extend({flags: ''}, options);
  var body = '([\\s\\S]+?)';

  open = open ? open : '\\${';
  close = close ? close : '}';

  return new RegExp(open + body + close, opts.flags);
};
