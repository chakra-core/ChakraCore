# Sort Object

[![Build Status](https://travis-ci.org/keithamus/sort-object-keys.svg)](https://travis-ci.org/keithamus/sort-object-keys)

Returns a copy of an object with all keys sorted.

The second argument is optional and is used for ordering - to provide custom sorts. You can either pass an array containing ordered keys or a function to sort the keys (same signature as in [`Array.prototype.sort()`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/sort)).

```js
const assert = require('assert');
const sortObject = require('sort-object-keys');

assert.equal(JSON.stringify({
  c: 1,
  b: 1,
  d: 1,
  a: 1,
}), JSON.stringify({
  a: 1,
  b: 1,
  c: 1,
  d: 1,
}));

assert.equal(JSON.stringify(sortObject({
  c: 1,
  b: 1,
  d: 1,
  a: 1,
}, ['b', 'a', 'd', 'c'])), JSON.stringify({
  b: 1,
  a: 1,
  d: 1,
  c: 1,
}));

function removeKeyAncCompareIndex(keyA, keyB){
  var a = parseInt(keyA.slice(4));
  var b = parseInt(keyB.slice(4));
  return a - b;
}

assert.equal(JSON.stringify(sortObject({
  "key-1": 1,
  "key-3": 1,
  "key-10": 1,
  "key-2": 1,
}, removeKeyAncCompareIndex)), JSON.stringify({
  "key-1": 1,
  "key-2": 1,
  "key-3": 1,
  "key-10": 1,
}));
```
