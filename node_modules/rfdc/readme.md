# rfdc

Really Fast Deep Clone

## Usage

```js
const clone = require('rfdc')()
clone({a: 1, b: {c: 2}}) // => {a: 1, b: {c: 2}}
```

## API

### `require('rfdc')(opts = { proto: false, circles: false }) => clone(obj) => obj2`

#### `proto` option

It's faster to allow enumerable properties on the prototype 
to be copied into the cloned object (not onto it's prototype,
directly onto the object).

To explain by way of code: 

```js
require('rfdc')({ proto: false })(Object.create({a: 1})) // => {}
require('rfdc')({ proto: true })(Object.create({a: 1})) // => {a: 1}
``` 

If this behavior is acceptable, set
`proto` to `true` for an additional 15% performance boost
(see benchmarks).

#### `circles` option

Keeping track of circular references will slow down performance
with an additional 40%-50% overhead (even if an object doesn't have
any circular references, the tracking is the cost). By default if 
an object with a circular reference is passed in, `rfdc` will throw (similar to
how `JSON.stringify` would throw). 

Use the `circles` option to detect and preserve circular references
in the object. If performance is important, try removing the 
circular reference from the object (set to `undefined`) and then
add it back manually after cloning instead of using this option.

### Types

`rdfc` clones all JSON types:

* `Object` 
* `Array`
* `Number`
* `String`
* `null`

With additional support for:

* `Date` (copied)
* `undefined` (copied)
* `Function` (referenced)
* `AsyncFunction` (referenced)
* `GeneratorFunction` (referenced)
* `arguments` (copied to a normal object)

All other types have output values that match the output
of `JSON.parse(JSON.stringify(o))`.

For instance: 

```js
const rdfc = require('rdfc')()
const err = Error()
err.code = 1
JSON.parse(JSON.stringify(e)) // {code: 1}
rdfc(e) // {code: 1}

JSON.parse(JSON.stringify(new Uint8Array([1, 2, 3]))) //  {'0': 1, '1': 2, '2': 3 }
rdfc(new Uint8Array([1, 2, 3])) //  {'0': 1, '1': 2, '2': 3 }

JSON.parse(JSON.stringify({rx: /foo/})) // {rx: {}}
rdfc({rx: /foo/}) // {rx: {}}
```

## Benchmarks

```sh
npm run bench
```

```
benchDeepCopy*100: 687.014ms
benchLodashCloneDeep*100: 1803.993ms
benchFastCopy*100: 929.259ms
benchRfdc*100: 565.133ms
benchRfdcProto*100: 484.401ms
benchRfdcCircles*100: 846.672ms
benchRfdcCirclesProto*100: 752.908ms
```

## Tests

```sh
npm test
```

```
148 passing (365.985ms)
```

### Coverage

```sh
npm run cov 
```

```
----------|----------|----------|----------|----------|-------------------|
File      |  % Stmts | % Branch |  % Funcs |  % Lines | Uncovered Line #s |
----------|----------|----------|----------|----------|-------------------|
All files |      100 |      100 |      100 |      100 |                   |
 index.js |      100 |      100 |      100 |      100 |                   |
----------|----------|----------|----------|----------|-------------------|
```

## License

MIT