# empty-dir [![Build Status](https://secure.travis-ci.org/js-cli/js-empty-dir.svg?branch=master)](http://travis-ci.org/js-cli/js-empty-dir)
> Check if a directory is empty.

[![NPM](https://nodei.co/npm/empty-dir.png)](https://nodei.co/npm/empty-dir/)

Note that directories with `.DS_Store` on mac are considered empty.

## Example
```js
const emptyDir = require('empty-dir');

emptyDir('./', function (err, result) {
  if (err) {
    console.error(err);
  } else {
    console.log('Directory is empty:', result);
  }
});

var result = emptyDir.sync('./test/empty');
console.log('Directory is empty:', result);
```

**Filter function**

Both async and sync take a filter function as the second argument.

_(This gives you the ability to eliminate files like `.DS_Store` on mac, or `Thumbs.db` on windows from causing the result to be "not empty" (`.DS_Store` is already filtered by default).)_

```js
const emptyDir = require('empty-dir');

function filter(filepath) {
  return !/Thumbs\.db$/i.test(filepath);
}

emptyDir('./', filter, function (err, result) {
  if (err) {
    console.error(err);
  } else {
    console.log('Directory is empty:', result);
  }
});

var result = emptyDir.sync('./test/empty', filter);
console.log('Directory is empty:', result);
```

## Release History

* 2014-05-08 - v0.1.0 - initial release
* 2016-02-07 - v0.2.0 - add filter support
