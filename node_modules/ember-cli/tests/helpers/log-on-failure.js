/* eslint mocha/no-top-level-hooks: 0 */

'use strict';

let logSink;

beforeEach(function() {
  logSink = [];
});

afterEach(function() {
  if (this.currentTest.state !== 'passed') {
    // It would be preferable to attach the log output to the error object
    // (this.currentTest.err) and have Mocha report it somehow, so that the
    // error message and log output show up in the same place. This doesn't
    // seem to be possible though.
    console.log(logSink.join('\n'));
  }
  logSink = null;
});

function logOnFailure(s) {
  if (logSink === null) {
    throw new Error('logOnFailure called outside of test');
  } else {
    logSink.push(s);
  }
}

module.exports = logOnFailure;
