# MatcherCollection [![Build Status](https://travis-ci.org/stefanpenner/matcher-collection.svg?branch=master)](https://travis-ci.org/stefanpenner/matcher-collection)

Minimatch but for collections of minimatcher matchers.

## Install

```sh
npm install matcher-collection
```

## Examples

```js
const MatcherCollection = require('matcher-collection')

let m = new MatcherCollection([
  'tests/',
  '**/*.js',
]);

m.match('tests/foo.js') // => true
m.match('foo.js')       // => false

m.mayContain('tests') // => true
m.mayContain('foo')   // => false
```
