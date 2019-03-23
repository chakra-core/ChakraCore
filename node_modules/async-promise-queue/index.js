'use strict';
var debug = require('debug')('async-promise-queue:');

// helper function that promisifies the queue
module.exports = function queue(worker, work, concurrency) {
  debug('started, with concurrency=' + concurrency)
  return new Promise(function(resolve, reject) {
    var q = require('async').queue(worker, concurrency);
    var firstError;
    q.drain = resolve;
    q.error = function(error) {
      if (firstError === undefined) {
        // only reject with the first error;
        firstError = error;
      }

      // don't start any new work
      q.kill();

      // but wait until all pending work completes before reporting
      q.drain = function() {
        reject(firstError);
      };

    };
    q.push(work);
  });
};

Object.defineProperty(module.exports, 'async', {
  get: function() {
    return require('async');
  }
});
