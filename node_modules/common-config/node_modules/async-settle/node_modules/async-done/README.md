async-done
==========

[![build status](https://secure.travis-ci.org/phated/async-done.png)](http://travis-ci.org/phated/async-done)

Handles completion and errors for callbacks, promises, observables and streams.

Will run call the function on `nextTick`. This will cause all functions to be async.

## Usage

### Successful completion

```js
var asyncDone = require('async-done');

asyncDone(function(done){
  // do async things
  done(null, 2);
}, function(error, result){
  // `error` will be undefined on successful execution of the first function.
  // `result` will be the result from the first function.
});
```

### Failed completion

```js
var asyncDone = require('async-done');

asyncDone(function(done){
  // do async things
  done(new Error('Some Error Occurred'));
}, function(error, result){
  // `error` will be an error from the first function.
  // `result` will be undefined on failed execution of the first function.
});
```

## API

### `asyncDone(fn, callback)`

Takes a function to execute (`fn`) and a function to call on completion (`callback`).

#### `fn([done])`

Optionally takes a callback to call when async tasks are complete.

#### Completion and Error Resolution

* `Callback` called
  - Completion: called with null error
  - Error: called with non-null error
* `Stream` or `EventEmitter` returned
  - Completion: [end-of-stream](https://www.npmjs.org/package/end-of-stream) module
  - Error: [domains](http://nodejs.org/api/domain.html)
* `Promise` returned
  - Completion: [onFulfilled](http://promisesaplus.com/#point-26) method called
  - Error: [onRejected](http://promisesaplus.com/#point-30) method called
* `Observable` returned
  - Completion: [onCompleted](https://github.com/Reactive-Extensions/RxJS/blob/master/doc/api/core/observable.md#rxobservableprototypesubscribeobserver--onnext-onerror-oncompleted) method called
  - Error: [onError](https://github.com/Reactive-Extensions/RxJS/blob/master/doc/api/core/observable.md#rxobservableprototypesubscribeobserver--onnext-onerror-oncompleted) method called

__Warning:__ Sync taks are __not supported__ and your function will never complete if the one of the above strategies is not used to signal completion. However, thrown errors will be caught by the domain.

#### `callback(error, result)`

If an error doesn't occur in the execution of the `fn` function, the `callback` method will receive the results as its second argument. Note: Observable and some streams don't received any results.

If an error occurred in the execution of the `fn` function, The `callback` method will receive an error as its first argument.

Errors can be caused by:

* A thrown error
* An error passed to a `done` callback
* An `error` event emitted on a returned `Stream` or `EventEmitter`
* A rejection of a returned `Promise`
* The `onError` handler being called on an `Observable`

## License

MIT
