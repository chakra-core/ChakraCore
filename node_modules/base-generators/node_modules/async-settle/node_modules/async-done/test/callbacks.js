'use strict';

var test = require('tap').test;

var asyncDone = require('../');

function success(cb){
  cb(null, 2);
}

function failure(cb){
  cb(new Error('Callback Error'));
}

function neverDone(someArg){
  return 2;
}

test('handle a successful callback', function(t){
  asyncDone(success, function(err, result){
    t.ok(err == null, 'error should be null or undefined');
    t.equal(result, 2, 'result should be 2');
    t.end();
  });
});

test('handle an errored callback', function(t){
  asyncDone(failure, function(err){
    t.ok(err instanceof Error, 'error should be instance of Error');
    t.end();
  });
});

test('a function that takes an argument but never calls callback', function(t){
  asyncDone(neverDone, function(err){
    t.plan(0);
    t.notOk(err);
  });

  setTimeout(function(){
    t.ok(true, 'done callback never called');
    t.end();
  }, 1000);
});

test('should not handle error if something throws inside the callback', function(t){
  // because tap timeout is a pain
  setTimeout(t.timeout.bind(t), 2000);

  t.plan(1);

  var d = require('domain').create();
  d.on('error', function(err){
    t.ok(err instanceof Error, 'error is not lost');
  });

  d.run(function(){
    asyncDone(success, function(){
      throw new Error('Thrown Error');
    });
  });
});
