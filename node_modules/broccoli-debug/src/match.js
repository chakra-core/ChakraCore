'use strict';
const regexpEscape = require('./util').regexpEscape;
const INCLUDE = [];
const EXCLUDE = [];

const PATTERN_SPLIT = /[\s,]+/;

let lastDebug;

function setDebug(debug) {
  if (debug === lastDebug) {
    return;
  }
  lastDebug = debug;

  let debugStr = typeof debug === 'string' ? debug : '';

  INCLUDE.length = 0;
  EXCLUDE.length = 0;

  debugStr.split(PATTERN_SPLIT).forEach((pattern) => {
    if (pattern.length === 0) {
      return;
    }
    let patterns;
    if (pattern[0] === '-') {
      pattern = pattern.slice(1);
      if (pattern.length === 0) {
        return;
      }
      patterns = EXCLUDE;
    } else {
      patterns = INCLUDE;
    }
    patterns.push(
      new RegExp('^' + pattern.split('*').map(regexpEscape).join('.*?') + '$') );
  });
}

module.exports = function match(label) {
  setDebug(process.env.BROCCOLI_DEBUG);

  for (let i = 0; i < EXCLUDE.length; i++) {
    if (EXCLUDE[i].test(label)) {
      return false;
    }
  }
  for (let i = 0; i < INCLUDE.length; i++) {
    if (INCLUDE[i].test(label)) {
      return true;
    }
  }
  return false;
}
