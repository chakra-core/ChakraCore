'use strict';
const path = require('path');
const REGEXP_ESCAPE = /[-\/\\^$*+?.()|[\]{}]/g;
// eslint-disable-next-line no-control-regex
const UNSAFE_PATH = /[\0-\x1f<>"\?\|\*]/g;
const PATH_SEP = /[:/\\]/g;

module.exports.regexpEscape = function regexpEscape(s) {
  return s.replace(REGEXP_ESCAPE, '\\$&');
}

function sanitizePath(input) {
  return input.replace(UNSAFE_PATH, '-').replace(PATH_SEP, '/');
}

module.exports.buildDebugOutputPath = function buildDebugOutputPath(options) {
  let label = sanitizePath(options.label);
  let debugOutputPath = path.join(options.baseDir, label);

  return debugOutputPath;
}
