bytewise-core
=============

Binary serialization of arbitrarily complex structures that sort element-wise

[![build status](https://travis-ci.org/deanlandolt/bytewise-core.svg?branch=master)](https://travis-ci.org/deanlandolt/bytewise-core)

Allows efficient comparison of a variety of useful data structures in a way that respects the sort order defined by [typewise](https://github.com/deanlandolt/typewise).

This library defines a total order for well-structured keyspaces in key value stores. The ordering is a superset of the sorting algorithm defined by [IndexedDB](http://www.w3.org/TR/IndexedDB/#key-construct) and the one defined by [CouchDB](http://wiki.apache.org/couchdb/View_collation). This serialization makes it easy to take advantage of the benefits of structured indexing in systems with fast but naïve binary indexing (key/value databases).


## Order of Supported Structures

This package is a barebones kernel of [bytewise](https://github.com/deanlandolt/bytewise), containing only the structures most often used to create structured keyspaces.

This is the top level order of the various structures that may be encoded:

* `null`
* `false`
* `true`
* `Number` (numeric)
* `Date` (time-wise)
* `Buffer`, `Uint8Array` (bit-wise)
* `String` (character-wise)
* `Array` (element-wise)
* `undefined`

Structured types like `Array` may recursively contain any other supported structures.


## Usage

`encode` serializes any supported type and returns a `Buffer`, or throws if an
unsupported structure is provided.

```js
var assert = require('assert')
var bytewise = require('./')
var encode = bytewise.encode

// Numbers are stored in 9 bytes -- 1 byte for the type tag and an 8 byte float
assert.equal(encode(12345).toString('hex'), '4240c81c8000000000')
// Negative numbers are stored as positive numbers, but with a lower type tag and their bits inverted
assert.equal(encode(-12345).toString('hex'), '41bf37e37fffffffff')

// The `toString` method of `Buffer` values returned by `encode` is augmented
// to use "hex" encoding by default. This ensures bytewise encoding still
// works when bytewise keys are accidentally coerced to strings.
assert.equal(encode(-12345) + '', '41bf37e37fffffffff')

// All numbers, integer or floating point, are stored as IEEE 754 doubles
assert.equal(encode(1.2345) + '', '423ff3c083126e978d')
assert.equal(encode(-1.2345) + '', '41c00c3f7ced916872')

// Serialization does not preserve the sign bit, so 0 is indistinguishable from -0
assert.equal(encode(-0) + '', '420000000000000000')
assert.equal(encode(0) + '', '420000000000000000')

// Strings are encoded as utf8, prefixed with their type tag (0x70, or the "p" character)
assert.equal(encode('foo').toString('utf8'), 'pfoo')
assert.equal(encode('föo').toString('utf8'), 'pföo')

// Arrays are just a series of values, separated by and terminated with a null byte
assert.equal(encode([ 'foo', 'bar' ]) + '', 'a070666f6f00706261720000')

// Items in arrays are delimited by null bytes, and a final end byte marks the end of the array
assert.equal(encode([ 'foo' ]).toString('binary'), '\xa0pfoo\x00\x00')

// Complex types like arrays can be arbitrarily nested, and fixed-sized types don't require a terminating byte
assert.equal(encode([ [ 'foo', 10 ], 'bar' ]) + '', 'a0a070666f6f0042402400000000000000706261720000')
```


`decode` parses a buffer and returns the structured data.
  
```js
var decode = bytewise.decode
var key = 'a0a070666f6f0042402400000000000000706261720000'

// Decode takes a buffer and decodes a bytewise value
assert.deepEqual(decode(new Buffer(key, 'hex')), [ [ 'foo', 10 ], 'bar' ])

// String input can be decoded, defaulting to hex
assert.deepEqual(decode(key), [ [ 'foo', 10 ], 'bar' ])

// An alternate string encoding can be provided when initializing bytewise
// TODO
```


## Use Cases

Take a look at the [bytewise](https://github.com/deanlandolt/bytewise#use-cases) library for an idea of what kind of stuff this could be useful for.


## Issues

Issues should be reported [here](https://github.com/deanlandolt/bytewise/issues).


## License

[MIT](http://deanlandolt.mit-license.org/)
