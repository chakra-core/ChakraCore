# now-and-later

Map over an array of values in parallel or series, passing each through the async iterator.
Optionally, specify lifecycle extension points for before the iterator runs, after completion,
or upon error.

## Usage

```js
var nal = require('now-and-later');

function iterator(value, cb){
  cb(null, value * 2)
}

function create(value, key){
  // return an object to be based to each lifecycle method
  return { key: key, value: value };
}

function before(storage){
  console.log('before iterator');
  console.log('initial value: ', storage.value);
}

function after(result, storage){
  console.log('after iterator');
  console.log('initial value: ', storage.value);
  console.log('result: ', result);
}

function error(err, storage){
  console.log('afer error in iterator');
  console.log('error: ', err);
}

/*
  Calling mapSeries with an object can't guarantee order
  It uses Object.keys to get an order
  It is better to use an array if order must be guaranteed
 */
nal.mapSeries([1, 2, 3], iterator {
  create: create,
  before: before,
  after: after,
  error: error
}, console.log);

nal.map({
  fn1: fn1,
  fn2: fn2
}, {
  create: create,
  before: before,
  after: after,
  error: error
}, console.log);
```
