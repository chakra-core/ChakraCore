'use strict';

var domain = require('domain');

var eos = require('end-of-stream');
var tick = require('next-tick');
var once = require('once');
var exhaust = require('stream-exhaust');

function noop(){}

var eosConfig = {
  error: false
};

function asyncDone(fn, cb){
  cb = once(cb);

  var d = domain.create();
  d.once('error', onError);
  var domainBoundFn = d.bind(fn);

  function done(error, result){
    d.removeListener('error', onError);
    d.exit();
    return cb(error, result);
  }

  function onSuccess(result){
    return done(null, result);
  }

  function onError(error){
    return done(error);
  }

  function asyncRunner(){
    var result = domainBoundFn(done);

    if(result && typeof result.on === 'function'){
      // assume node stream
      d.add(result);
      eos(exhaust(result), eosConfig, onSuccess);
      return;
    }

    if(result && typeof result.subscribe === 'function'){
      // assume RxJS observable
      result.subscribe(noop, onError, onSuccess);
      return;
    }

    if(result && typeof result.then === 'function'){
      // assume promise
      result.then(onSuccess, onError);
      return;
    }
  }

  tick(asyncRunner);
}

module.exports = asyncDone;
