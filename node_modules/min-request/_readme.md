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
