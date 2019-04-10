'use strict';

var fs = require('fs');
var lazy = require('lazy-cache')(require);
lazy('kind-of', 'typeOf');
lazy('rethrow');
lazy('engine');

function error(str, options) {
  options = options || {};

  var fp = options.fp || 'string';
  var data = options.data || {};
  var re = options.regex || /<%-([\s\S]+?)%>|<%=([\s\S]+?)%>|\$\{([^\\}]*(?:\\.[^\\}]*)*)\}|<%([\s\S]+?)%>|$/g;
  var regex = toRegexp(re);

  str.replace(regex, function(match, prop, i) {
    try {
      var engine = lazy.engine();
      engine.render(str, data);
    } catch (err) {
      var rethrow = lazy.rethrow({});
      var prop = matchProp(err.message);
      var match = findProp(prop, str, regex);
      var before = str.slice(0, match.index);
      var lineno = before.split('\n').length;
      rethrow(err, fp, lineno, str);
    }
  });
};

function matchProp(str) {
  var re = /(\w+) is not defined/;
  var m = re.exec(str);
  if (!m) return;
  return m[1];
}

function findProp(prop, str, re) {
  var src = re.source;
  var left = /^([-{<\[=%]+)/.exec(src);
  var esc = '(?:[' + left[0].split('').join('\\') + '\\s]+)';
  var delim = esc + '.*' + prop;
  var re = new RegExp(delim, 'm');
  return re.exec(str);
}

function toRegexp(val) {
  if (lazy.typeOf(val) === 'regexp') {
    return val;
  }
  if (lazy.typeOf(val) === 'object') {
    return val.interpolate;
  }
}

function toSettings(val) {
  if (lazy.typeOf(val) === 'regexp') {
    return { interpolate: val };
  }
  if (lazy.typeOf(val) === 'object') {
    return val;
  }
  return null;
}

/**
 * Expose `error`
 */

module.exports = error;
