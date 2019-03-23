## Usage

### With no Heimdall Tree

```js
var logger = require('heimdalljs-logger')('foo');

logger.trace('trace message');
logger.debug('debug message');
logger.info('info message');
logger.warn('warn message');
logger.error('error message');

console.log('app message');
```

```sh
DEBUG=foo DEBUG_LEVEL=trace foo
# =>  trace message
# ... debug message
# ... info message
# ... warn message
# ... error message
# ... app message

foo
# =>  app message

DEBUG=foo DEBUG_LEVEL=warn foo
# =>  warn message
# ... error message
# ... app message
```

### With a Heimdall Tree

```js
var heimdall = require('heimdalljs');
var config  = require('heimdalljs').configFor('logging');

config.matcher = (id) => /hai/.test(id.name);
config.depth = 2;

var logger = require('heimdalljs-logger')('foo');

heimdall.start('a');
heimdall.start('bhai');
heimdall.start('c');
heimdall.start('dhai');

logger.warn('warn message');
// => foo [bhai -> dhai] warn message
```
