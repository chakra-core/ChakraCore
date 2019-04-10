'use strict';

var utils = require('./utils');
var invertedCache;
var matchersCache;

module.exports = function processArgs(app, argv) {
  // temporary hack to ensure that shortcut keys are parsed correctly
  // this should be fixed in the parser (minimist or parse-args)
  var inverted = invertedCache || (invertedCache = utils.invert(utils.aliases));
  var matchers = matchersCache || (matchersCache = createMatchers(inverted));

  var len = argv.length;
  var idx = -1;

  while (++idx < len) {
    argv[idx] = replaceFlag(argv[idx], matchers);
  }

  return app.argv(argv, {
    esc: utils.fileKeys,
    first: ['init', 'new', 'ask', 'emit', 'global', 'save', 'config', 'file'],
    last: ['tasks']
  });
};

function createMatchers(obj) {
  var res = {};
  for (var key in obj) {
    var val = obj[key];
    res[key] = {};
    res[key].re = new RegExp('(?:^|)-' + key + '(?!\\w)', 'g');
    res[key].val = '--' + val;
  }
  return res;
}

function replaceFlag(str, matchers) {
  for (var key in matchers) {
    var matcher = matchers[key];

    if (matcher.re.test(str)) {
      return str.replace(matcher.re, matcher.val);
    }
  }
  return str;
}
