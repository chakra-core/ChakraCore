# p-some [![Build Status](https://travis-ci.org/sindresorhus/p-some.svg?branch=master)](https://travis-ci.org/sindresorhus/p-some)

> Wait for a specified number of promises to be fulfilled

Useful when you need the fastest of multiple promises.


## Install

```
$ npm install p-some
```


## Usage

Checks 4 websites and logs the 2 fastest.

```js
const got = require('got');
const pSome = require('p-some');

pSome([
	got.head('github.com').then(() => 'github'),
	got.head('google.com').then(() => 'google'),
	got.head('twitter.com').then(() => 'twitter'),
	got.head('medium.com').then(() => 'medium')
], {count: 2}).then(([first, second]) => {
	console.log(first, second);
	//=> 'google twitter'
});
```


## API

### pSome(input, options)

Returns a `Promise` that is fulfilled when `count` promises from `input` are fulfilled. The fulfilled value is an `Array` of the values from the `input` promises in the order they were fulfilled. If it becomes impossible to satisfy `count`, for example, too many promises rejected, it will reject with an [`AggregateError`](https://github.com/sindresorhus/aggregate-error) error.

#### input

Type: `Iterable<Promise|any>`

#### options

Type: `Object`

##### count

*Required*<br>
Type: `number` *(minimum `1`)*

##### filter

Type: `Function`

Receives the value resolved by the promise. Used to filter out values that doesn't satisfy a condition.

### pSome.AggregateError

Exposed for instance checking.


## Related

- [p-any](https://github.com/sindresorhus/p-any) - Wait for any promise to be fulfilled
- [More…](https://github.com/sindresorhus/promise-fun)


## License

MIT © [Sindre Sorhus](https://sindresorhus.com)
