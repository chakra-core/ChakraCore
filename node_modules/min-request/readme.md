min-request
===

[![Build status][travis-image]][travis-url]
[![Test coverage][coveralls-image]][coveralls-url]
[![NPM version][npm-image]][npm-url]
[![Downloads][downloads-image]][downloads-url]

Simple request, For people who cannot understand [request](https://github.com/request/request) like me to use http request

Installation
---

```sh
npm install min-request
```

Support
---

Support body types

- string
- json
- stream
- buffer

Usage
---

request(url, [options], callback)

callback param is just like `request@request`: err, res, body

Simplest

```js
var request = require('min-request')
request('localhost:8080/test', function(err, res, body) {
    console.log(err, body)
})
```

Request with data like json, stream

```js
var request = require('min-request')

// json
request('localhost:8080/upload', {
    method: 'POST',
    body: {foo: 'bar'}
},function(err, res, body) {
    // ...
})

// stream
var fs = require('fs')
request('localhost:8080/upload', {
    method: 'POST',
    body: fs.createReadStream('./foo.bar')
}, function(err, res, body) {
    // ...
})
```

Advanced
---

use `NODE_DEBUG=request` to show request options

License
---

ISC

[npm-image]: https://img.shields.io/npm/v/min-request.svg?style=flat-square
[npm-url]: https://npmjs.org/package/min-request
[travis-image]: https://img.shields.io/travis/chunpu/min-request.svg?style=flat-square
[travis-url]: https://travis-ci.org/chunpu/min-request
[coveralls-image]: https://img.shields.io/coveralls/chunpu/min-request.svg?style=flat-square
[coveralls-url]: https://coveralls.io/r/chunpu/min-request
[downloads-image]: http://img.shields.io/npm/dm/min-request.svg?style=flat-square
[downloads-url]: https://npmjs.org/package/min-request
