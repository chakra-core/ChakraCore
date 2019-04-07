# unique-random [![Build Status](https://travis-ci.org/sindresorhus/unique-random.svg?branch=master)](https://travis-ci.org/sindresorhus/unique-random)

> Generate random numbers that are consecutively unique.

Useful for eg. slideshows where you don't want to have the same slide twice in a row.


## Install

```sh
$ npm install --save unique-random
```


## Usage

```js
var rand = require('unique-random')(1, 10);
console.log(rand(), rand(), rand());
//=> 5 2 6
```


## API

### uniqueRandom(*min*, *max*)

Returns a function which when called will return a random number that's never the same as the previous number.


## License

MIT © [Sindre Sorhus](http://sindresorhus.com)
