"use strict";

/**
 * Takes in a processor function, and returns a writer function.
 *
 * @param {Function} processor
 *
 * @return {Function} queuedWriter
 */
module.exports = function createQueuedWriter(processor) {
  var queue = [];

  var iterator = function iterator(callback) {
    return function (err) {
      queue.shift();
      callback(err);

      var next = queue[0];
      if (next) {
        processor(next.fs, next.data, iterator(next.callback));
      }
    };
  };

  return function queuedWriter(fs, data, callback) {
    var empty = !queue.length;
    queue.push({ fs: fs, data: data, callback: callback });

    if (empty) {
      // start processing
      processor(fs, data, iterator(callback));
    }
  };
};