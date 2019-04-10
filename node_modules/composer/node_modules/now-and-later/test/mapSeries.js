'use strict';

var lab = exports.lab = require('lab').script();
var describe = lab.describe;
var it = lab.it;
var before = lab.before;
var beforeEach = lab.beforeEach;
var after = lab.after;
var afterEach = lab.afterEach;
var expect = require('lab').expect;

var nal = require('../');

describe('mapSeries', function(){

  it('will execute without an extension object', function(done){
    var initial = [1, 2, 3];

    function iterator(value, cb){
      cb(null, value);
    }

    nal.mapSeries(initial, iterator, function(err, result){
      expect(initial).to.deep.equal(result);
      done(err);
    });
  });

  it('should execute without a final callback', function(done){
    var initial = [1, 2, 3];
    var result = [];

    function iterator(value, cb){
      result.push(value);
      if(result.length === initial.length){
        expect(initial).to.deep.equal(result);
        done();
      }
      cb(null, value);
    }

    nal.mapSeries(initial, iterator);
  });

  it('should execute with array', function(done){
    var initial = [1, 2, 3];

    function iterator(value, cb){
      cb(null, value);
    }

    nal.mapSeries(initial, iterator, function(err, result){
      expect(initial).to.deep.equal(result);
      done(err);
    });
  });

  it('should execute with an object', function(done){
    var initial = {
      test: 1,
      test2: 2,
      test3: 3
    };

    function iterator(value, cb){
      cb(null, value);
    }

    nal.mapSeries(initial, iterator, function(err, result){
      expect(initial).to.deep.equal(result);
      done(err);
    });
  });

  it('should throw if first argument is a non-object', function(done){
    function nonObject(){
      nal.mapSeries('nope');
    }

    expect(nonObject).to.throw(Error);
    done();
  });

  it('should maintain order', function(done){
    var callOrder = [];

    function iterator(value, cb){
      setTimeout(function(){
        callOrder.push(value);
        cb(null, value * 2);
      }, value * 25);
    }

    nal.mapSeries([1, 3, 2], iterator, function(err, result){
      expect(callOrder).to.deep.equal([1, 3, 2]);
      expect(result).to.deep.equal([2, 6, 4]);
      done(err);
    });
  });

  it('should not mutate the original array', function(done){
    var initial = [1, 2, 3];

    function iterator(value, cb){
      cb(null, value);
    }

    nal.mapSeries(initial, iterator, function(err, result){
      expect(initial).to.not.equal(result);
      expect(initial).to.deep.equal(result);
      done(err);
    });
  });

  it('should fail when an error occurs', function(done){
    function iterator(value, cb){
      cb(new Error('Boom'));
    }

    nal.mapSeries([1, 2, 3], iterator, function(err, results){
      expect(err).to.be.an.instanceof(Error);
      expect(err.message).to.equal('Boom');
      done();
    });
  });

  it('should ignore multiple calls to the callback inside iterator', function(done){
    var initial = [1, 2, 3];

    function iterator(value, cb){
      cb(null, value);
      cb(null, value * 2);
    }

    nal.mapSeries(initial, iterator, function(err, result){
      expect(initial).to.deep.equal(result);
      done(err);
    });
  });

  it('should take extension points and call them for each function', function(done){
    var initial = [1, 2, 3];
    var create = [];
    var before = [];
    var after = [];

    function iterator(value, cb){
      cb(null, value);
    }

    var extensions = {
      create: function(value, idx){
        expect(initial).to.include(value);
        create[idx] = value;
        return { idx: idx, value: value };
      },
      before: function(storage){
        before[storage.idx] = storage.value;
      },
      after: function(result, storage){
        after[storage.idx] = result;
      }
    };

    nal.mapSeries(initial, iterator, extensions, function(err, result){
      expect(initial).to.deep.equal(create);
      expect(initial).to.deep.equal(before);
      expect(result).to.deep.equal(after);
      done(err);
    });
  });

  it('should call the error extension point upon error', function(done){
    var initial = [1, 2, 3];
    var error = [];

    function iterator(value, cb){
      cb(new Error('Boom'));
    }

    var extensions = {
      create: function(){
        return {};
      },
      error: function(err){
        error = err;
      }
    };

    nal.mapSeries(initial, iterator, extensions, function(err){
      expect(err).to.deep.equal(error);
      done();
    });
  });

  it('should pass an empty object if falsy value is returned from create', function(done){
    var initial = [1, 2, 3];

    function iterator(value, cb){
      cb(null, value);
    }

    var extensions = {
      create: function(){
        return null;
      },
      before: function(storage){
        expect(storage).to.be.an('object');
        expect(storage).to.deep.equal({});
      }
    };

    nal.mapSeries(initial, iterator, extensions, done);
  });
});
