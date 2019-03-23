var assert = require('assert'),
    Promise = require('../lib/Promise'),
    Pool = require('../lib/Pool');

function add(a, b) {
  return a + b;
}


function tryRequire(moduleName) {
  try {
    return require(moduleName);
  } catch(error) {
    if (typeof error === 'object' && error !== null && error.code == 'MODULE_NOT_FOUND') {
      return null;
      // no worker_threads, fallback to sub-process based workers
    } else {
      throw error;
    }
  }
}

describe('Pool', function () {

  describe('nodeWorker', function() {
    function add(a,b) {
      return a+b;
    }
    it('supports process', function() {
      var pool = new Pool({ nodeWorker: 'process' });
      return pool.exec(add, [3, 4])
    });

    var WorkerThreads = tryRequire('worker_threads');
    it('supports auto', function() {
      var pool = new Pool({ nodeWorker: 'auto' });
      var result = pool.exec(add, [3, 4]);
      assert.equal(pool.workers.length, 1);
      var worker = pool.workers[0].worker;

      if (WorkerThreads) {
        assert.equal(worker.isWorkerThread, true);
      } else {
        assert.equal(worker.isChildProcess, true);
      }
      return result;
    });


    if (WorkerThreads) {
      it('supports thread', function() {
        var pool = new Pool({ nodeWorker: 'thread' });
        var work = pool.exec(add, [3, 4]);
        assert.equal(pool.workers.length, 1);
        var worker = pool.workers[0].worker;
        assert.equal(worker.isWorkerThread, true);
        return work;
      });
    } else {
      it('errors when not supporting worker thread', function() {
        assert.throws(function() {
          var pool = new Pool({ nodeWorker: 'thread' });
        }, /WorkerPool: nodeWorkers = thread is not supported, Node >= 11\.7\.0 required/)
      });
    }
  })

  it('should offload a function to a worker', function (done) {
    var pool = new Pool({maxWorkers: 10});

    function add(a, b) {
      return a + b;
    }

    assert.equal(pool.workers.length, 0);

    pool.exec(add, [3, 4])
        .then(function (result) {
          assert.equal(result, 7);
          assert.equal(pool.workers.length, 1);

          pool.clear();

          assert.equal(pool.workers.length, 0);

          done();
        });

    assert.equal(pool.workers.length, 1);
  });

  it('should offload functions to multiple workers', function (done) {
    var pool = new Pool({maxWorkers: 10});

    function add(a, b) {
      return a + b;
    }

    assert.equal(pool.workers.length, 0);

    Promise.all([
          pool.exec(add, [3, 4]),
          pool.exec(add, [2, 3])
        ])
        .then(function (results) {
          assert.deepEqual(results, [7, 5]);
          assert.equal(pool.workers.length, 2);

          pool.clear();
          done();
        });

    assert.equal(pool.workers.length, 2);
  });

  it('should put tasks in queue when all workers are busy', function (done) {
    var pool = new Pool({maxWorkers: 2});

    function add(a, b) {
      return a + b;
    }

    assert.equal(pool.tasks.length, 0);
    assert.equal(pool.workers.length, 0);

    var task1 = pool.exec(add, [3, 4]);
    var task2 = pool.exec(add, [2, 3]);

    assert.equal(pool.tasks.length, 0);
    assert.equal(pool.workers.length, 2);

    var task3 = pool.exec(add, [5, 7]);
    var task4 = pool.exec(add, [1, 1]);

    assert.equal(pool.tasks.length, 2);
    assert.equal(pool.workers.length, 2);

    Promise.all([
        task1,
        task2,
        task3,
        task4
        ])
        .then(function (results) {
          assert.deepEqual(results, [7, 5, 12, 2]);
          assert.equal(pool.tasks.length, 0);
          assert.equal(pool.workers.length, 2);

          pool.clear();
          done();
        });
  });

  it('should create a proxy', function (done) {
    var pool = new Pool();

    pool.proxy().then(function (proxy) {
      assert.deepEqual(Object.keys(proxy).sort(), ['methods', 'run']);

      proxy.methods()
          .then(function (methods) {
            assert.deepEqual(methods.sort(), ['methods', 'run']);
            done();
          })
          .catch(function (err) {
            assert('Should not throw an error');
          });
    });
  });

  it('should create a proxy of a custom worker', function (done) {
    var pool = new Pool(__dirname + '/workers/simple.js');

    pool.proxy().then(function (proxy) {
      assert.deepEqual(Object.keys(proxy).sort(), ['add','methods','multiply','run','timeout']);

      done();
    });
  });

  it('should invoke a method via a proxy', function (done) {
    var pool = new Pool(__dirname + '/workers/simple.js');

    pool.proxy().then(function (proxy) {
      proxy.multiply(4, 3)
          .then(function (result) {
            assert.equal(result, 12);
            done();
          })
          .catch(function (err) {
            console.log(err);
            assert('Should not throw an error');
          });
    });
  });

  it('should invoke an async method via a proxy', function (done) {
    var pool = new Pool(__dirname + '/workers/simple.js');

    pool.proxy().then(function (proxy) {
      proxy.timeout(100)
          .then(function (result) {
            assert.equal(result, 'done');
            done();
          })
          .catch(function (err) {
            console.log(err);
            assert('Should not throw an error');
          });
    });
  });

  it('should handle errors thrown by a worker', function (done) {
    var pool = new Pool({maxWorkers: 10});

    function test() {
      throw new TypeError('Test error');
    }

    pool.exec(test)
        .catch(function (err) {
          assert.ok(err instanceof Error);
          assert.equal(err.message, 'Test error')

          pool.clear();
          done();
        });
  });

  it('should execute a function returning a Promise', function (done) {
    var pool = new Pool({maxWorkers: 10});

    function testAsync() {
      return Promise.resolve('done');
    }

    pool.exec(testAsync)
        .then(function (result) {
          assert.equal(result, 'done');
          done();
        })
        .catch(function (err) {
          assert('Should not throw an error');
        });
  });

  it('should propagate a rejected Promise', function (done) {
    var pool = new Pool({maxWorkers: 10});

    function testAsync() {
      return Promise.reject(new Error('I reject!'));
    }

    pool.exec(testAsync)
        .then(function (result) {
          assert('Should not resolve');
        })
        .catch(function (err) {
          //assert.ok(err instanceof Error);  // FIXME: returned error should be an instanceof Error
          assert.equal(err, err.toString('Error: I reject!'));
          done();
        });
  });

  it('should cancel a task', function (done) {
    var pool = new Pool({maxWorkers: 10});

    function forever() {
      while (1 > 0) {} // runs forever
    }

    var promise = pool.exec(forever)
        .then(function (result) {
          assert('promise should never resolve');
        })
      //.catch(Promise.CancellationError, function (err) { // TODO: not yet supported
        .catch(function (err) {
          assert(err instanceof Promise.CancellationError);

          assert.equal(pool.workers.length, 0);

          done();
        });

    // cancel the task
    setTimeout(function () {
      promise.cancel();
    }, 0);
  });

  it('should cancel a queued task', function (done) {
    var pool = new Pool({maxWorkers: 1});
    var reachedTheEnd = false;

    function delayed() {
      var Promise = require('../lib/Promise');

      return new Promise(function (resolve, reject) {
        setTimeout(function () {
          resolve(1);
        }, 0);
      });
    }

    function one() {
      return 1;
    }

    var p1 = pool.exec(delayed)
        .then(function (result) {
          assert.equal(result, 1);
          assert.equal(reachedTheEnd, true);

          assert.equal(pool.workers.length, 1);
          assert.equal(pool.tasks.length, 0);

          done();
        });

    assert.equal(pool.workers.length, 1);
    assert.equal(pool.tasks.length, 0);

    var p2 = pool.exec(one); // will be queued
    assert.equal(pool.workers.length, 1);
    assert.equal(pool.tasks.length, 1);

    p2.cancel();            // cancel immediately
    assert.equal(pool.workers.length, 1);
    assert.equal(pool.tasks.length, 1);

    reachedTheEnd = true;
  });

    it('should run following tasks if a previous queued task is cancelled', function (done) {

        var pool = new Pool({maxWorkers: 1});
        var reachedTheEnd = false;

        function delayed() {
            var Promise = require('../lib/Promise');

            return new Promise(function (resolve, reject) {
                setTimeout(function () {
                    resolve(1);
                }, 0);
            });
        }

        function two() {
            var Promise = require('../lib/Promise');

            return new Promise(function (resolve, reject) {
                setTimeout(function () {
                    resolve(2);
                }, 0);
            });
        }

        function one() {
            return 1;
        }

        var oneDone = false;
        var twoDone = false;
        function checkDone() {
            if (oneDone && twoDone) {
                done();
            }
        }

        var p1 = pool.exec(delayed)
        .then(function (result) {
            assert.equal(result, 1);
            assert.equal(reachedTheEnd, true);

            oneDone = true;

            assert.equal(pool.workers.length, 1);
            assert.equal(pool.tasks.length, 1);

            checkDone();
        });

        assert.equal(pool.workers.length, 1);
        assert.equal(pool.tasks.length, 0);

        var p2 = pool.exec(one); // will be queued
        assert.equal(pool.workers.length, 1);
        assert.equal(pool.tasks.length, 1);

        var p3 = pool.exec(two)
        .then(function (result) {
            assert.equal(result, 2);
            assert.equal(reachedTheEnd, true);

            twoDone = true;

            assert.equal(pool.workers.length, 1);
            assert.equal(pool.tasks.length, 0);

            checkDone();
        });

        p2.cancel();            // cancel immediately
        assert.equal(pool.workers.length, 1);
        assert.equal(pool.tasks.length, 2);

        reachedTheEnd = true;
    });

    // TODO: test whether a task in the queue can be neatly cancelled

  it('should timeout a task', function (done) {
    var pool = new Pool({maxWorkers: 10});

    function forever() {
      while (1 > 0) {} // runs forever
    }

    var promise = pool.exec(forever)
        .timeout(50)
        .then(function (result) {
          assert('promise should never resolve');
        })
      //.catch(Promise.CancellationError, function (err) { // TODO: not yet supported
        .catch(function (err) {
          assert(err instanceof Promise.TimeoutError);

          assert.equal(pool.workers.length, 0);

          done();
        });
  });

  it('should start timeout timer of a task once the task is taken from the queue (1)', function (done) {
    var pool = new Pool({maxWorkers: 1});
    var delay = 50

    function sleep() {
      return new Promise(function (resolve, reject) {
        setTimeout(function () {
          resolve ('done :)')
        }, 100) // 2 * delay
      })
    }

    function doNothing() {
      return 'ready'
    }

    // add a task
    pool.exec(sleep)

    // add a second task, will be queued until the first finishes
    // the timeout is shorter than the currently executing task and longer than
    // the queued task, so it should not timeout
    pool.exec(doNothing)
        .timeout(delay)
        .then(function (result) {
          assert.equal(result, 'ready');

          done();
        })
        .catch(function (err) {
          assert('promise should not throw');
        });
  });

  it('should start timeout timer of a task once the task is taken from the queue (2)', function (done) {
    var pool = new Pool({maxWorkers: 1});
    var delay = 50

    function sleep() {
      return new Promise(function (resolve, reject) {
        setTimeout(function () {
          resolve ('done :)')
        }, 100) // 2 * delay
      })
    }

    // add a task
    pool.exec(sleep)

    // add a second task, will be queued until the first finishes
    pool.exec(sleep)
        .timeout(delay)
        .then(function (result) {
          assert('promise should never resolve');
        })
        .catch(function (err) {
          assert(err instanceof Promise.TimeoutError);

          done();
        });
  });

  it('should handle crashed workers (1)', function (done) {
    var pool = new Pool({maxWorkers: 1});

    pool.exec(add)
        .then(function () {
          assert('Promise should not be resolved');
        })
      .catch(function (err) {
          assert.ok(err.toString().match(/Error: Workerpool Worker terminated Unexpectedly/));
          assert.ok(err.toString().match(/exitCode: `.*`/));
          assert.ok(err.toString().match(/signalCode: `.*`/));
          assert.ok(err.toString().match(/workerpool.script: `.*\.js`/));
          assert.ok(err.toString().match(/spawnArgs: `.*\.js`/));
          assert.ok(err.toString().match(/spawnfile: `.*node`/));
          assert.ok(err.toString().match(/stdout: `null`/));
          assert.ok(err.toString().match(/stderr: `null`/));

          assert.equal(pool.workers.length, 0);

          // validate whether a new worker is spawned
          pool.exec(add, [2,3])
              .then(function (result) {
                assert.equal(result, 5);

                assert.equal(pool.workers.length, 1);

                pool.clear();
                done();
              });

          assert.equal(pool.workers.length, 1);
        });

    assert.equal(pool.workers.length, 1);

    // kill the worker so it will be terminated
    pool.workers[0].worker.kill();

    assert.equal(pool.workers.length, 1);
  });

  describe('options', function () {

    it('should throw an error on invalid type or number of maxWorkers', function () {
      assert.throws(function () {
        new Pool({maxWorkers: 'a string'});
      }, TypeError);

      assert.throws(function () {
        new Pool({maxWorkers: 2.5});
      }, TypeError);

      assert.throws(function () {
        new Pool({maxWorkers: 0});
      }, TypeError);

      assert.throws(function () {
        new Pool({maxWorkers: -1});
      }, TypeError);
    });

    it('should limit to the configured number of max workers', function () {
      var pool = new Pool({maxWorkers: 2});

      pool.exec(add, [1, 2]);
      pool.exec(add, [3, 4]);
      pool.exec(add, [5, 6]);
      pool.exec(add, [7, 8]);
      pool.exec(add, [9, 0]);

      assert.equal(pool.maxWorkers, 2);
      assert.equal(pool.workers.length, 2);
      assert.equal(pool.tasks.length, 3);

      pool.clear();
    });

    it('should take number of cpus minus one as default maxWorkers', function () {
      var pool = new Pool();

      var cpus = require('os').cpus();
      assert.equal(pool.maxWorkers, cpus.length - 1);

      pool.clear();
    });

    it('should throw an error on invalid type or number of minWorkers', function () {
      assert.throws(function () {
        new Pool({minWorkers: 'a string'});
      }, TypeError);

      assert.throws(function () {
        new Pool({minWorkers: 2.5});
      }, TypeError);

      assert.throws(function () {
        new Pool({maxWorkers: -1});
      }, TypeError);
    });

    it('should create number of cpus minus one when minWorkers set to \'max\'', function () {
      var pool = new Pool({minWorkers:'max'});

      var cpus = require('os').cpus();
      assert.equal(pool.workers.length, cpus.length - 1);

      pool.clear();
    });

    it('should increase maxWorkers to match minWorkers', function () {
      var pool = new Pool({minWorkers: 16});

      for(var i=0;i<20;i++) pool.exec(add, [i, i*2]);

      assert.equal(pool.minWorkers, 16);
      assert.equal(pool.maxWorkers, 16);
      assert.equal(pool.workers.length, 16);
      assert.equal(pool.tasks.length, 4);

      pool.clear();
    });
  });

  it.skip('should handle crashed workers (2)', function (done) {
    // TODO: create a worker from a script, which really crashes itself
  });

  it('should clear all workers', function (done) {
    var pool = new Pool({maxWorkers: 10});

    assert.equal(pool.workers.length, 0);

    function test() {
      return 'ok';
    }

    pool.exec(test)
        .then(function (result) {
          assert.equal(result, 'ok');

          assert.equal(pool.workers.length, 1);

          pool.clear();

          assert.equal(pool.workers.length, 0);

          done();
        });

    assert.equal(pool.workers.length, 1);
  });

  it('should clear all workers after tasks are finished', function (done) {
    var pool = new Pool({maxWorkers: 10});

    assert.equal(pool.workers.length, 0);

    function test() {
      return 'ok';
    }

    pool.exec(test)
        .then(function (result) {
          assert.equal(result, 'ok');

          assert.equal(pool.workers.length, 0);
        });
    assert.equal(pool.workers.length, 1);

    pool.terminate(false, 1000)
      .then(function() {
        assert.equal(pool.workers.length, 0);
        done();
      });
  });

  it ('should wait for all workers if pool is terminated before multiple concurrent tasks are finished', function (done) {

    var pool = new Pool({maxWorkers: 10});

    assert.equal(pool.workers.length, 0);

    function test1() {
      var start = new Date().getTime();
      for (var i = 0; i < 1e7; i++) {
        if ((new Date().getTime() - start) > 500) {
          break;
        }
      }
      return 'test 1 ok';
    }
    function test2() {
      var start = new Date().getTime();
      for (var i = 0; i < 1e7; i++) {
        if ((new Date().getTime() - start) > 1000) {
          break;
        }
      }
      return 'test 2 ok';
    }
    function test3() {
      var start = new Date().getTime();
      for (var i = 0; i < 1e7; i++) {
        if ((new Date().getTime() - start) > 200) {
          break;
        }
      }
      return 'test 3 ok';
    }

    var promises = [
      pool.exec(test1),
      pool.exec(test2),
      pool.exec(test3)
    ];
    Promise.all(promises)
      .then(function (results) {
        assert.equal(results[0], 'test 1 ok');
        assert.equal(results[1], 'test 2 ok');
        assert.equal(results[3], 'test 3 ok');
      })
      .catch(function(error) {
        assert.fail(error);
      });
    assert.equal(pool.workers.length, 3);

    pool.terminate(false, 2000)
      .then(function() {
        assert.equal(pool.workers.length, 0);
        done();
      });
  });

  it ('should wait for all workers if pool is terminated before tasks are finished, even if a task fails', function (done) {

    var pool = new Pool({maxWorkers: 10});

    assert.equal(pool.workers.length, 0);

    function test1() {
      var start = new Date().getTime();
      for (var i = 0; i < 1e7; i++) {
        if ((new Date().getTime() - start) > 1000) {
          break;
        }
      }
      return 'test 1 ok';
    }
    function test2() {
      var start = new Date().getTime();
      for (var i = 0; i < 1e7; i++) {
        if ((new Date().getTime() - start) > 100) {
          break;
        }
      }
      throw new Error('test 2 error');
    }

    var promises = [
      pool.exec(test1),
      pool.exec(test2)
    ];
    Promise.all(promises)
      .then(function (results) {
        assert.fail('test2 should have been rejected');
      })
      .catch(function(error) {
        assert.equal(error.message, 'test 2 error');
      });
    assert.equal(pool.workers.length, 2);

    pool.terminate(false, 2000)
      .then(function() {
        assert.equal(pool.workers.length, 0);
        done();
      });
  });

  it('should return statistics', function () {
    var pool = new Pool({maxWorkers: 4});

    function test() {
      return new Promise(function (resolve, reject) {
        setTimeout(resolve, 100);
      });
    }

    function testError() {
      return new Promise(function (resolve, reject) {
        throw new Error('Test error')
      });
    }

    assert.deepEqual(pool.stats(), {totalWorkers: 0, busyWorkers: 0, idleWorkers: 0, pendingTasks: 0, activeTasks: 0});

    var promise = pool.exec(test)
        .then(function () {
          assert.deepEqual(pool.stats(), {totalWorkers: 1, busyWorkers: 0, idleWorkers: 1, pendingTasks: 0, activeTasks: 0 });

          // start six tasks (max workers is 4, so we should get pending tasks)
          var all = Promise.all([
            pool.exec(test),
            pool.exec(test),
            pool.exec(test),
            pool.exec(test),
            pool.exec(test),
            pool.exec(test)
          ]);

          assert.deepEqual(pool.stats(), {totalWorkers: 4, busyWorkers: 4, idleWorkers: 0, pendingTasks: 2, activeTasks: 4});

          return all;
        })
        .then(function () {
          assert.deepEqual(pool.stats(), {totalWorkers: 4, busyWorkers: 0, idleWorkers: 4, pendingTasks: 0, activeTasks: 0 });

          return pool.exec(testError)
        })
        .catch(function () {
          assert.deepEqual(pool.stats(), {totalWorkers: 4, busyWorkers: 0, idleWorkers: 4, pendingTasks: 0, activeTasks: 0});
        });

    assert.deepEqual(pool.stats(), {totalWorkers: 1, busyWorkers: 1, idleWorkers: 0, pendingTasks: 0, activeTasks: 1});

    return promise;
  });

  it('should throw an error in case of wrong type of arguments in function exec', function () {
    var pool = new Pool();
    assert.throws(function () {pool.exec()}, TypeError);
    assert.throws(function () {pool.exec(23)}, TypeError);
    assert.throws(function () {pool.exec(add, {})}, TypeError);
    assert.throws(function () {pool.exec(add, 2, 3)}, TypeError);
    assert.throws(function () {pool.exec(add, 'a string')}, TypeError);
  });

});
