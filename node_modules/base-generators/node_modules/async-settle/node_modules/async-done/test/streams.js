'use strict';

var fs = require('fs');
var path = require('path');

var test = require('tap').test;
var through = require('through2');

var asyncDone = require('../');

var exists = path.join(__dirname, '../.gitignore');
var notExists = path.join(__dirname, '../not_exists');

var EndStream = through.ctor(function(chunk, enc, cb){
  this.push(chunk);
  cb();
}, function(cb){
  this.emit('end', 2);
  cb();
});

function success(){
  var read = fs.createReadStream(exists);
  return read.pipe(new EndStream());
}

function failure(){
  var read = fs.createReadStream(notExists);
  return read.pipe(new EndStream());
}

function unpiped(){
  return fs.createReadStream(exists);
}

test('handle a successful stream', function(t){
  asyncDone(success, function(err){
    t.ok(err == null, 'error should be null or undefined');
    t.end();
  });
});

test('handle an errored stream', function(t){
  asyncDone(failure, function(err){
    t.ok(err instanceof Error, 'error should be instance of Error');
    t.ok(err.domainEmitter, 'error should be caught by domain');
    t.end();
  });
});

test('handle a returned stream and cb by only calling callback once', function(t){
  asyncDone(function(cb){
    return success().on('end', function(){ cb(); });
  }, function(err){
    t.ok(err == null, 'error should be null or undefined');
    t.end();
  });
});

test('consumes an unpiped readable stream', function(t){
  asyncDone(unpiped, function(err){
    t.ok(err == null, 'error should be null or undefined');
    t.end();
  });
});
