bach
====

[![build status](https://secure.travis-ci.org/gulpjs/bach.png)](http://travis-ci.org/gulpjs/bach)

Compose your async functions with elegance

## Usage

With Bach, it is very easy to compose async functions to run in series or parallel.

```js
var bach = require('bach');

function fn1(cb){
  cb(null, 1);
}

function fn2(cb){
  cb(null, 2);
}

function fn3(cb){
  cb(null, 3);
}

var seriesFn = bach.series(fn1, fn2, fn3);
// fn1, fn2, and fn3 will be run in series
seriesFn(function(err, res){
  if(err){ // in this example, err is undefined
    // handle error
  }
  // handle results
  // in this example, res is [1, 2, 3]
});

var parallelFn = bach.parallel(fn1, fn2, fn3);
// fn1, fn2, and fn3 will be run in parallel
parallelFn(function(err, res){
  if(err){ // in this example, err is undefined
    // handle error
  }
  // handle results
  // in this example, res is [1, 2, 3]
});
```

Since the composer functions just return a function that can be called, you can combine them.

```js
var combinedFn = bach.series(fn1, bach.parallel(fn2, fn3));
// fn1 will be executed before fn2 and fn3 are run in parallel
combinedFn(function(err, res){
  if(err){ // in this example, err is undefined
    // handle error
  }
  // handle results
  // in this example, res is [1, [2, 3]]
});
```

Functions are called with [async-done](https://github.com/gulpjs/async-done), so you can return a stream or promise.
The function will complete when the stream ends/closes/errors or the promise fulfills/rejects.

```js
// streams
var fs = require('fs');

function streamFn1(){
  return fs.createReadStream('./example')
    .pipe(fs.createWriteStream('./example'));
}

function streamFn2(){
  return fs.createReadStream('./example')
    .pipe(fs.createWriteStream('./example'));
}

var parallelStreams = bach.parallel(streamFn1, streamFn2);
parallelStreams(function(err){
  if(err){ // in this example, err is undefined
    // handle error
  }
  // all streams have emitted an 'end' or 'close' event
});
```

```js
// promises
var when = require('when');

function promiseFn1(){
  return when.resolve(1);
}

function promiseFn2(){
  return when.resolve(2);
}

var parallelPromises = bach.parallel(promiseFn1, promiseFn2);
parallelPromises(function(err, res){
  if(err){ // in this example, err is undefined
    // handle error
  }
  // handle results
  // in this example, res is [1, 2]
});
```

All errors are caught in a [domain](http://nodejs.org/api/domain.html) and passed to the final callback as the first argument.

```js
function success(cb){
  setTimeout(function(){
    cb(null, 1);
  }, 500);
}

function error(){
  throw new Error('Thrown Error');
}

var errorThrownFn = bach.parallel(error, success);
errorThrownFn(function(err, res){
  if(err){
    // handle error
    // in this example, err is an error caught by the domain
  }
  // handle results
  // in this example, res is [undefined]
});
```

Something that may be encountered when an error happens in a parallel composition is the callback
will be called as soon as the error happens. If you want to continue on error and wait until all
functions have finished before calling the callback, use `settleSeries` or `settleParallel`.

```js
function success(cb){
  setTimeout(function(){
    cb(null, 1);
  }, 500);
}

function error(cb){
  cb(new Error('Async Error'));
}

var parallelSettlingFn = bach.settleParallel(success, error);
parallelSettlingFn(function(err, res){
  // all functions have finished executing
  if(err){
    // handle error
    // in this example, err is an error passed to the callback
  }
  // handle results
  // in this example, res is [1]
});
```

## API

__All bach APIs return an invoker function that takes a single callback as its only parameter.
The function signature is `function(error, results)`.__

Each method can optionally be passed an object of [extension point functions](#extension-points)
as the last argument.

### `series(fns..., [extensions])` => Function

All functions (`fns`) passed to this function will be called in series when the returned function is
called.  If an error occurs, execution will stop and the error will be passed to the callback function
as the first parameter.

__The error parameter will always be a single error.__

### `parallel(fns..., [extensions])` => Function

All functions (`fns`) passed to this function will be called in parallel when the returned
function is called.  If an error occurs, the error will be passed to the callback function
as the first parameter. Any async functions that have not completed, will still complete,
but their results will __not__ be available.

__The error parameter will always be a single error.__

### `settleSeries(fns..., [extensions])` => Function

All functions (`fns`) passed to this function will be called in series when the returned function is
called. All functions will always be called and the callback will receive all settled errors and results.

__The error parameter will always be an array of errors.__

### `settleParallel(fns..., [extensions])` => Function

All functions (`fns`) passed to this function will be called in parallel when the returned function is
called. All functions will always be called and the callback will receive all settled errors and results.

__The error parameter will always be an array of errors.__

### Extension Points

An extension point object can contain:

#### `create(fn, key)` => `storage` object

Called before the async function or extension points are called. Receives the function and key to be
executed in the future.  The return value should be any object and will be passed to the other extension
point methods.  The storage object can keep any information needed between extension points and can
be mutated within extension points.

#### `before(storage)`

Called before the async function is executed. Receives the storage object returned from the `create`
extension point.

#### `after(storage)`

Called after the async function is executed and completes successfully. Receives the storage object
returned from the `create` extension point.

#### `error(storage)`

Called after the async function is executed and errors. Receives the storage object returned from
the `create` extension point.
