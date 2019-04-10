# typewise

Typewise structured sorting for arbirarily complex data structures.

[![build status](https://travis-ci.org/deanlandolt/typewise.svg?branch=master)](https://travis-ci.org/deanlandolt/typewise)

This library defines and implements the collation used by the [bytewise](https://github.com/deanlandolt/bytewise-core) encoding library.

NOTE: the core typewise sorting functionality has been completely rewritten and moved to [typewise-core](https://github.com/deanlandolt/typewise-core). This library extends the data structures and comparators available to support more exotic types like ordered maps and sets, and shortlex-ordered tuples and records.


## Type system

In order to properly express the rules for sorting and equality for a wide range of structures `typewise` defines a simple type system for controlling these properties across a range of data structures.

A `typewise` type profile can be provide when creating a bytewise codec to control encoding and decoding behavior for specific types. Types may also contain high and low sentinal values that can be used to create `range` types which may be impossible be instantiate directly as instances.

For example, dates have no valid infinatary instances, but something analogous to the "minimum" and "maximum" dates is a useful construct for defining date intervals.


## Issues

Issues should be reported [here](https://github.com/deanlandolt/bytewise/issues).


## License

[MIT](http://deanlandolt.mit-license.org/)
