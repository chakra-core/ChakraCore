'use strict';

var utils = require('./utils');
var resolve = require('./resolve');

function flowFactory(flow) {
  return function(/* list of tasks/functions to compose */) {
    var args = [].concat.apply([], [].slice.call(arguments));
    var self = this;
    return function(done) {
      if (typeof done !== 'function') {
        done = function(err) {
          if (err) {
            self.emit('error', err);
          }
        };
      }
      var fns;
      try {
        fns = resolve.call(self, args);
      } catch (err) {
        return done(err);
      }
      if (fns.length === 1) {
        return fns[0](done);
      }

      var batch;
      try {
        batch = utils.bach[flow].apply(utils.bach, fns);
      } catch (err) {
        return done(err);
      }
      return batch(done);
    };
  };
};

module.exports = flowFactory;
