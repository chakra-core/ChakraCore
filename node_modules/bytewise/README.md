bytewise
========

Binary serialization of arbitrarily complex structures that sort element-wise

[![build status](https://travis-ci.org/deanlandolt/bytewise.svg?branch=master)](https://travis-ci.org/deanlandolt/bytewise)

Allows efficient comparison of a variety of useful data structures in a way that respects the sort order defined by [typewise](https://github.com/deanlandolt/typewise).

The [bytewise-core](https://github.com/deanlandolt/bytewise-core) library defines a total order for well-structured keyspaces in key value stores. The ordering is a superset of the sorting algorithm defined by [IndexedDB](http://www.w3.org/TR/IndexedDB/#key-construct) and the one defined by [CouchDB](http://wiki.apache.org/couchdb/View_collation). This serialization makes it easy to take advantage of the benefits of structured indexing in systems with fast but naïve binary indexing (key/value databases).


## Order of Supported Structures

This is the top level order of the various structures that may be encoded:

* `null`
* `false`
* `true`
* `Number` (numeric)
* `Date` (numeric, epoch offset)
* `Buffer` (bitwise)
* `String` (lexicographic)
* `Array` (componentwise)
* `undefined`


These specific structures can be used to serialize the vast majority of javascript values in a way that can be sorted in an efficient, complete and sensible manner. Each value is prefixed with a type tag, and we do some bit munging to encode our values in such a way as to carefully preserve the desired sort behavior, even in the presence of structural nested.

For example, negative numbers are stored as a different *type* from positive numbers, with its sign bit stripped and its bytes inverted to ensure numbers with a larger magnitude come first. `Infinity` and `-Infinity` can also be encoded -- they are *nullary* types, encoded using just their type tag. The same can be said of `null` and `undefined`, and the boolean values `false`, `true`. `Date` instances are stored just like `Number` instances -- but as in IndexedDB -- `Date` sorts after `Number` (including `Infinity`). `Buffer` data can be stored in the raw, and is sorted before `String` data. Then come the collection types (just `Array` for the time being).


## Unsupported Structures

This serialization accommodates a wide range of javascript structures, but it is not exhaustive. Complex structures with reference cycles cannot be serialized. `NaN` is also illegal anywhere in a serialized value -- its presence very likely indicates of an error, but more importantly sorting on `NaN` is nonsensical by definition. Objects which are instances of `Error` are also rejected, as well as `Invalid Date` objects. If and when we support more complex collection types, `WeakMap` and `WeakSet` objects will never be serializable as they cannot be enumerated. Attempts to serialize any values which include these structures will throw an error.


## Usage

`encode` serializes any supported type and returns a buffer, or throws if an
unsupported structure is provided.
  
  ```js
  var assert = require('assert');
  var bytewise = require('./');
  var encode = bytewise.encode;

  // Many types can be represented using only their type tag, a single byte
  assert.equal(encode(null).toString('binary'), '\x10');
  assert.equal(encode(false).toString('binary'), '\x20');
  assert.equal(encode(true).toString('binary'), '\x21');
  assert.equal(encode(undefined).toString('binary'), '\xf0');

  // Numbers are stored in 9 bytes -- 1 byte for the type tag and an 8 byte float
  assert.equal(encode(12345).toString('hex'), '4240c81c8000000000');
  // Negative numbers are stored as positive numbers, but with a lower type tag and their bits inverted
  assert.equal(encode(-12345).toString('hex'), '41bf37e37fffffffff');

  // The `toString` method of `Buffer` values returned by `encode` is augmented
  // to use "hex" encoding by default. This ensures bytewise encoding still
  // works when bytewise keys are accidentally coerced to strings.
  assert.equal(encode(true) + '', '21');

  // All numbers, integer or floating point, are stored as IEEE 754 doubles
  assert.equal(encode(1.2345) + '', '423ff3c083126e978d');
  assert.equal(encode(-1.2345) + '', '41c00c3f7ced916872');

  // Serialization does not preserve the sign bit, so 0 is indistinguishable from -0
  assert.equal(encode(-0) + '', '420000000000000000');
  assert.equal(encode(0) + '', '420000000000000000');

  // We can even serialize Infinity and -Infinity, though we just use their type tag
  assert.equal(encode(-Infinity) + '', '40');
  assert.equal(encode(Infinity) + '', '43');

  // Dates are stored just like numbers, but with different (and higher) type tags
  assert.equal(encode(new Date(-12345)) + '', '51bf37e37fffffffff');
  assert.equal(encode(new Date(12345)) + '', '5240c81c8000000000');

  // Strings are encoded as utf8, prefixed with their type tag (0x70, or the "p" character)
  assert.equal(encode('foo').toString('utf8'), 'pfoo');
  assert.equal(encode('föo').toString('utf8'), 'pföo');

  // Buffers are also left alone, other than being prefixed with their type tag (0x60)
  assert.equal(encode(new Buffer('ff00fe01', 'hex')) + '', '60ff00fe01');

  // Arrays are just a series of values terminated with a null byte
  assert.equal(encode([ true, -1.2345 ]) + '', 'a02141c00c3f7ced91687200');

  // Strings are also legible when embedded in complex structures like arrays
  // Items in arrays are delimited by null bytes, and a final end byte marks the end of the array
  assert.equal(encode([ 'foo' ]).toString('binary'), '\xa0pfoo\x00\x00');

  // The 0x01 and 0xfe bytes are used to escape high and low bytes while preserving the correct collation
  assert.equal(encode([ new Buffer('ff00fe01', 'hex') ]) + '', 'a060fefe0101fefd01020000');

  // Complex types like arrays can be arbitrarily nested, and fixed-sized types don't require a terminating byte
  assert.equal(encode([ [ 'foo', true ], 'bar' ]).toString('binary'), '\xa0\xa0\pfoo\x00\x21\x00\pbar\x00\x00');

  // Objects are just string-keyed maps, stored like arrays: [ k1, v1, k2, v2, ... ]
  // NYI in this version
  // assert.equal(encode({ foo: true, bar: 'baz' }).toString('binary'), '\xb0pfoo\x00\x21\pbar\x00\pbaz\x00\x00');
  
  ```


`decode` parses a buffer and returns the structured data, or throws if malformed:
  
  ```js
  var samples = [
    'foo √',
    null,
    '',
    new Date('2000-01-01T00:00:00Z'),
    42,
    undefined,
    [ undefined ],
    -1.1,
    {},
    [],
    true,
    { bar: 1 },
    [ { bar: 1 }, { bar: [ 'baz' ] } ],
    -Infinity,
    false
  ];
  var result = samples.map(bytewise.encode).map(bytewise.decode);
  assert.deepEqual(samples, result);
  ```


`compare` is just a convenience bytewise comparison function:

  ```js
  var sorted = [
    null,
    false,
    true,
    -Infinity,
    -1.1,
    42,
    new Date('2000-01-01Z'),
    '',
    'foo √',
    [],
    [ { bar: 1 }, { bar: [ 'baz' ] } ],
    [ undefined ],
    {},
    { bar: 1 },
    undefined
  ];

  var result = samples.map(bytewise.encode).sort(bytewise.compare).map(bytewise.decode);
  assert.deepEqual(sorted, result);
  ```


## Use Cases

### Numeric indexing

This is surprisingly difficult to with vanilla LevelDB -- basic approaches require ugly hacks like left-padding numbers to make them sort lexicographically (which is prone to overflow problems). You could write a one-off comparator function in C, but there a number of drawbacks to this as well. This serialization solves this problem in a clean and generalized way, in part by taking advantage of properties of the byte sequences defined by the IEE 754 floating point standard.

### Namespaces, partitions and patterns

This is another really basic and oft-needed amenity that isn't very easy out of the box in LevelDB. We reserve the lowest and highest bytes as abstract tags representing low and high key sentinels, allowing you to faithfully request all values in any portion of an array. Arrays can be used as namespaces without any leaky hacks, or even more detailed slicing can be done per element to implement wildcards or even more powerful pattern semantics for specific elements in the array keyspace.

### Document storage

It may be reasonably fast to encode and decode, but `JSON.stringify` isn't terribly useful or objects as document records in a way that is useful for range queries, where LevelDB and its ilk excel. This serialization allows you to build indexes on top of your documents, as well as expanding on the range of serializable types available from JSON.

### Multilevel language-sensitive collation

You have a bunch of strings in a particular language-specific strings you want to index, but at the time of indexing you're not sure *how* sorted you need them. Queries may or may not care about case or punctuation differences, for instance. You can index your string as an array of weights, most-to-least specific, and prefixed by collation language (since our values are language-sensitive). There are [mechanisms available](http://www.unicode.org/reports/tr10/#Run-length_Compression) to compress this array to keep its size reasonable.

### Full-text search

Full-text indexing is a natural extension of the language-sensitive collation use case described above. Add a little lexing and stemming and basic full text search is close at hand. Structured indexes can be employed to make other more interesting search features possible as well.

### CouchDB-style "joins"

Build a view that colocates related subrecords, taking advantage of component-wise sorting of arrays to interleave them. This is a technique [employed by CouchDB](http://www.cmlenz.net/archives/2007/10/couchdb-joins), leveraging its very similar collation semantics to keep related grouped together hierarchically. More recently [Akiban](http://www.akiban.com/) has formalized this concept of a [table grouping](http://blog.akiban.com/how-does-table-grouping-compare-to-sql-server-indexed-views/) and brought it the SQL world. Again, bytewise sorting extends naturally to their notions of [hierarchical keys](http://blog.akiban.com/introducing-hkey/).

### Emulating other systems

Clients that wish to employ a subset of the full range of possible types above can preprocess values to coerce them into the desired simpler forms before serializing. For instance, if you were to build CouchDB-style indexing you could round-trip values through a `JSON` encode cycle (to get just the subset of types supported by CouchDB) before passing to `encode`, resulting in a collation that is identical to CouchDB. Emulating IndexedDB's collation would at least require preprocessing away `Buffer` data and `undefined` values and normalizing for the es6 types.


## Issues

Issues should be reported [here](https://github.com/deanlandolt/bytewise/issues).


## License

[MIT](http://deanlandolt.mit-license.org/)
