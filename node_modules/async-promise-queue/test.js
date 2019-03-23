'use strict';
var queue = require('./');
var chai = require('chai');
var expect = chai.expect;

describe('behavior', function() {
  // the example worker
  var worker = queue.async.asyncify(function(work) {
    return new Promise(function(resolve) {
      work.cb();
      setTimeout(resolve, work.duration);
    });
  });

  it('handles the run away work problem (all fail)', function() {
    var one, two, three, four = false;
    var thrownError = new Error('some error');
    // the work
    var work = [
      {
        cb: function() {
          one = true;
          throw thrownError;
        },
        file:'/path-1',
        duration: 0
      },
      {
        cb: function() {
          two = true;
          throw thrownError;
        },
        file:'/path-2',
        duration: 0
      },
      {
        cb: function() {
          three = true;
          throw thrownError;
        },
        file:'/path-3',
        duration: 0
      },

      {
        cb: function() {
          four = true;
        },
        file:'/path-4',
        duration: 0
      },
    ];

    // calling our queue helper
    return queue(worker, work, 3)
      .then(function(value) { return expect(true).to.eql(false); }, // should never get here
        function(reason) { return expect(reason).to.eql(thrownError); })
      .then(function() {
        // assert the work we expect to be done is done, but nothing more
        expect(one, 'first job should have executed').to.eql(true);
        expect(two, 'second job should have executed').to.eql(true);
        expect(three, 'third job should have executed').to.eql(true);
        expect(four, 'fourth job should have executed').to.eql(false);
      });
  });

  it('handles the run away work problem', function() {

    var one, two, three, four = false;
    var thrownError = new Error('some error');
    // the work
    var work = [
      {
        cb: function() {
          one = true;
        },
        file:'/path-1',
        duration: 50
      },
      {
        cb: function() {
          two = true;
          throw thrownError;
        },
        file:'/path-2',
        duration: 0
      },
      {
        cb: function() {
          three = true;
        },
        file:'/path-3',
        duration: 0
      },

      {
        cb: function() {
          four = true;
        },
        file:'/path-4',
        duration: 50
      },
    ];

    // calling our queue helper
    return queue(worker, work, 3)
      .then(function(value) { return expect(true).to.eql(false); }, // should never get here
        function(reason) { return expect(reason).to.eql(thrownError); })
      .then(function() {
        // assert the work we expect to be done is done, but nothing more
        expect(one, 'first job should have executed').to.eql(true);
        expect(two, 'second job should have executed').to.eql(true);
        expect(three, 'third job should have executed').to.eql(true);
        expect(four, 'fourth job should have executed').to.eql(false);
      });
  });

  it('works correctly when everything is wonderful', function() {
    // the example worker
    var one, two, three, four = false;

    // the work
    var work = [
      {
        cb: function() {
          one = true;
        },
        file: '/path-1',
        duration: 50
      },
      {
        cb: function() {
          two = true;
        },
        file: '/path-2',
        duration: 0
      },
      {
        cb: function() {
          three = true;
        },
        file: '/path-3',
        duration: 0
      },
      {
        cb: function() {
          four = true;
        },
        file: '/path-4',
        duration: 50
      },
    ];

    // calling our queue helper
    return queue(worker, work, 3)
      .then(function(value) { return expect(value).to.eql(undefined); })
      .then(function() {
        // assert the work we expect to be done is done, but nothing more
        expect(one,   'first job should have executed').to.eql(true);
        expect(two,   'second job should have executed').to.eql(true);
        expect(three, 'third job should have executed').to.eql(true);
        expect(four,  'fourth job should have executed').to.eql(true);
      });
  });
});
