# humanize-string [![Build Status](https://travis-ci.org/sindresorhus/humanize-string.svg?branch=master)](https://travis-ci.org/sindresorhus/humanize-string)

> Converts a camelized/dasherized/underscored string into a humanized one  
> Example: `fooBar-Baz_Faz` → `Foo bar baz faz`


## Install

```
$ npm install --save humanize-string
```


## Usage

```js
const humanizeString = require('humanize-string');

humanizeString('fooBar');
//=> 'Foo bar'

humanizeString('foo-bar');
//=> 'Foo bar'

humanizeString('foo_bar');
//=> 'Foo bar'
```


## License

MIT © [Sindre Sorhus](http://sindresorhus.com)
