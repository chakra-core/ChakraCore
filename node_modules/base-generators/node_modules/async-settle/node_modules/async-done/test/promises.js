'use strict';

var test = require('tap').test;
var when = require('when');

var asyncDone = require('../');

function success(){
  return when.resolve(2);
}

function failure(){
  return when.reject(new Error('Promise Error'));
}

test('handle a resolved promise', function(t){
  asyncDone(success, function(err, result){
    t.ok(err == null, 'error should be null or undefined');
    t.equal(result, 2, 'result should be 2');
    t.end();
  });
});

test('handle a rejected promise', function(t){
  asyncDone(failure, function(err){
    t.ok(err instanceof Error, 'error should be instance of Error');
    t.end();
  });
});
