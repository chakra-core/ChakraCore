
# find-index

An implementation of the ES6 method `Array.prototype.findIndex` as a standalone module and a 
[*ponyfill*](https://ponyfill.com).

Finds an item in an array matching a predicate function, and returns its index.

Fast both when `thisArg` is used and also when it isn't.

### usage

```bash
npm install find-index
```

```js
var findIndex = require('find-index/findIndex')
var findIndex = require('find-index/ponyfill') // will use native Array#findIndex if available.
var findLastIndex = require('find-index/findLastIndex') // search backwards from end
```
    findIndex(array, callback[, thisArg])
    findLastIndex(array, callback[, thisArg])
    Parameters:
      array
        The array to operate on.
      callback
        Function to execute on each value in the array, taking three arguments:
          element
            The current element being processed in the array.
          index
            The index of the current element being processed in the array.
          array
            The array findIndex was called upon.
      thisArg
        Object to use as this when executing callback.

Based on [array-findindex](https://www.npmjs.org/package/array-findindex)

### performance

```bash
$ iojs --harmony_arrays perf/benchmark.js

native Array.prototype.findIndex: 6347ms
findIndex: 1633ms
findIndex ponyfill: 6384ms
findLastIndex: 1508ms
npm lodash.findindex: 2900ms
npm array-findindex: 3512ms
```
