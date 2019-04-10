'use strict';

var test = require('tap').test;

var settle = require('../');

test('should transform success into settled success values', function(t){
  var val = 'value to be settled';
  settle(function(done){
    done(null, val);
  }, function(error, result){
    t.notOk(error, 'error should be undefined');
    t.ok(result, 'result should be defined');
    t.ok(result.state === 'success', 'state should be "success"');
    t.equal(result.value, val, 'value should be the success value');
    t.end();
  });
});

test('should transform errors into settled success values', function(t){
  var err = new Error('Error to be settled');
  settle(function(done){
    done(err);
  }, function(error, result){
    t.notOk(error, 'error should be undefined');
    t.ok(result, 'result should be defined');
    t.ok(result.state === 'error', 'state should be "error"');
    t.equal(result.value, err, 'value should be the error value');
    t.end();
  });
});
