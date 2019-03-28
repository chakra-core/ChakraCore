# base-store [![NPM version](https://img.shields.io/npm/v/base-store.svg?style=flat)](https://www.npmjs.com/package/base-store) [![NPM downloads](https://img.shields.io/npm/dm/base-store.svg?style=flat)](https://npmjs.org/package/base-store) [![Build Status](https://img.shields.io/travis/node-base/base-store.svg?style=flat)](https://travis-ci.org/node-base/base-store)

Plugin for getting and persisting config values with your base-methods application. Adds a 'store' object that exposes all of the methods from the data-store library. Also now supports sub-stores!

You might also be interested in [base-data](https://github.com/node-base/base-data).

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install base-store --save
```

## Usage

Adds `store` methods for doing things like this:

```js
app.store.set('a', 'z'); // DOES persist
console.log(app.store.get('a'));
//=> 'z';
```

## API

Add a `.store` method to your [base](https://github.com/node-base/base) application:

```js
var store = require('base-store');
var Base = require('base');
var base = new Base();

// store `name` is required
base.use(store('foo'));

// optionally define a cwd to use for persisting the store
// default cwd is `~/data-store/`
base.use(store('foo', {cwd: 'a/b/c'}));
```

**example usage**

```js
base.store
  .set('a', 'b')
  .set({c: 'd'})
  .set('e.f', 'g')

console.log(base.store.get('e.f'));
//=> 'g'

console.log(base.store.data);
//=> {a: 'b', c: 'd', e: {f: 'g'}}
```

## Sub-stores

A sub-store is a custom store that is persisted to its own file in a sub-folder of its "parent" store.

**Create a sub-store**

```js
app.store.create('foo');
// creates an instance of store on `app.store.foo`

app.store.foo.set('a', 'b');
app.store.foo.get('a');
//=> 'b'
```

Sub-store data is also persisted to a property on the "parent" store:

```js
// set data on a sub-store
app.store.foo.set('a', 'b');

// get the value from parent store
app.store.get('foo.a');
//=> 'b'
```

### plugin params

* `name` **{String}**: Store name.
* `options` **{Object}**

* `cwd` **{String}**: Current working directory for storage. If not defined, the user home directory is used, based on OS. This is the only option currently, other may be added in the future.
* `indent` **{Number}**: Number passed to `JSON.stringify` when saving the data. Defaults to `2` if `null` or `undefined`

## methods

### .store.set

Assign `value` to `key` and save to disk. Can be a key-value pair or an object.

**Params**

* `key` **{String}**
* `val` **{any}**: The value to save to `key`. Must be a valid JSON type: String, Number, Array or Object.
* `returns` **{Object}** `Store`: for chaining

**Example**

```js
// key, value
base.store.set('a', 'b');
//=> {a: 'b'}

// extend the store with an object
base.store.set({a: 'b'});
//=> {a: 'b'}

// extend the the given value
base.store.set('a', {b: 'c'});
base.store.set('a', {d: 'e'}, true);
//=> {a: {b 'c', d: 'e'}}

// overwrite the the given value
base.store.set('a', {b: 'c'});
base.store.set('a', {d: 'e'});
//=> {d: 'e'}
```

### .store.union

Add or append an array of unique values to the given `key`.

**Params**

* `key` **{String}**
* `returns` **{any}**: The array to add or append for `key`.

**Example**

```js
base.store.union('a', ['a']);
base.store.union('a', ['b']);
base.store.union('a', ['c']);
base.store.get('a');
//=> ['a', 'b', 'c']
```

### .store.get

Get the stored `value` of `key`, or return the entire store if no `key` is defined.

**Params**

* `key` **{String}**
* `returns` **{any}**: The value to store for `key`.

**Example**

```js
base.store.set('a', {b: 'c'});
base.store.get('a');
//=> {b: 'c'}

base.store.get();
//=> {b: 'c'}
```

### .store.has

Returns `true` if the specified `key` has truthy value.

**Params**

* `key` **{String}**
* `returns` **{Boolean}**: Returns true if `key` has

**Example**

```js
base.store.set('a', 'b');
base.store.set('c', null);
base.store.has('a'); //=> true
base.store.has('c'); //=> false
base.store.has('d'); //=> false
```

### .store.hasOwn

Returns `true` if the specified `key` exists.

**Params**

* `key` **{String}**
* `returns` **{Boolean}**: Returns true if `key` exists

**Example**

```js
base.store.set('a', 'b');
base.store.set('b', false);
base.store.set('c', null);
base.store.set('d', true);

base.store.hasOwn('a'); //=> true
base.store.hasOwn('b'); //=> true
base.store.hasOwn('c'); //=> true
base.store.hasOwn('d'); //=> true
base.store.hasOwn('foo'); //=> false
```

### .store.save

Persist the store to disk.

**Params**

* `dest` **{String}**: Optionally define a different destination than the default path.

**Example**

```js
base.store.save();
```

### .store.del

Delete `keys` from the store, or delete the entire store if no keys are passed. A `del` event is also emitted for each key deleted.

**Note that to delete the entire store you must pass `{force: true}`**

**Params**

* `keys` **{String|Array|Object}**: Keys to remove, or options.
* `options` **{Object}**

**Example**

```js
base.store.del();

// to delete paths outside cwd
base.store.del({force: true});
```

## History

**v0.3.1**

* Sub-stores are easier to create and get. You can now do `app.store.create('foo')` to create a sub-store, which is then available as `app.store.foo`.

**v0.3.0**

* Introducing sub-stores!

## Related projects

Other plugins for extending your [base](https://github.com/node-base/base) application:

* [base-options](https://www.npmjs.com/package/base-options): Adds a few options methods to base-methods, like `option`, `enable` and `disable`. See the readme… [more](https://www.npmjs.com/package/base-options) | [homepage](https://github.com/jonschlinkert/base-options)
* [base-pipeline](https://www.npmjs.com/package/base-pipeline): base-methods plugin that adds pipeline and plugin methods for dynamically composing streaming plugin pipelines. | [homepage](https://github.com/node-base/base-pipeline)
* [base-plugins](https://www.npmjs.com/package/base-plugins): Upgrade's plugin support in base applications to allow plugins to be called any time after… [more](https://www.npmjs.com/package/base-plugins) | [homepage](https://github.com/node-base/base-plugins)
* [base-questions](https://www.npmjs.com/package/base-questions): Plugin for base-methods that adds methods for prompting the user and storing the answers on… [more](https://www.npmjs.com/package/base-questions) | [homepage](https://github.com/node-base/base-questions)
* [base](https://www.npmjs.com/package/base): base is the foundation for creating modular, unit testable and highly pluggable node.js applications, starting… [more](https://www.npmjs.com/package/base) | [homepage](https://github.com/node-base/base)

## Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](https://github.com/node-base/base-store/issues/new).

## Building docs

Generate readme and API documentation with [verb](https://github.com/verbose/verb):

```sh
$ npm install verb && npm run docs
```

Or, if [verb](https://github.com/verbose/verb) is installed globally:

```sh
$ verb
```

## Running tests

Install dev dependencies:

```sh
$ npm install -d && npm test
```

## Author

**Jon Schlinkert**

* [github/jonschlinkert](https://github.com/jonschlinkert)
* [twitter/jonschlinkert](http://twitter.com/jonschlinkert)

## License

Copyright © 2016, [Jon Schlinkert](https://github.com/jonschlinkert).
Released under the [MIT license](https://github.com/node-base/base-store/blob/master/LICENSE).

***

_This file was generated by [verb](https://github.com/verbose/verb), v0.9.0, on May 19, 2016._