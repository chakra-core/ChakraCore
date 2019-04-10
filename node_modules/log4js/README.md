# log4js-node [![Build Status](https://secure.travis-ci.org/log4js-node/log4js-node.png?branch=master)](http://travis-ci.org/log4js-node/log4js-node) [![codecov](https://codecov.io/gh/log4js-node/log4js-node/branch/master/graph/badge.svg)](https://codecov.io/gh/log4js-node/log4js-node)


[![NPM](https://nodei.co/npm/log4js.png?downloads=true&downloadRank=true&stars=true)](https://nodei.co/npm/log4js/)

This is a conversion of the [log4js](https://github.com/stritti/log4js)
framework to work with [node](http://nodejs.org). I started out just stripping out the browser-specific code and tidying up some of the javascript to work better in node. It grew from there. Although it's got a similar name to the Java library [log4j](https://logging.apache.org/log4j/2.x/), thinking that it will behave the same way will only bring you sorrow and confusion.

The full documentation is available [here](https://log4js-node.github.io/log4js-node/).

[Changes in version 3.x](https://log4js-node.github.io/log4js-node/v3-changes.md)

There have been a few changes between log4js 1.x and 2.x (and 0.x too). You should probably read this [migration guide](https://log4js-node.github.io/log4js-node/migration-guide.html) if things aren't working.

Out of the box it supports the following features:

* coloured console logging to stdout or stderr
* file appender, with configurable log rolling based on file size or date
* a logger for connect/express servers
* configurable log message layout/patterns
* different log levels for different log categories (make some parts of your app log as DEBUG, others only ERRORS, etc.)

Optional appenders are available:
* [SMTP](https://github.com/log4js-node/smtp)
* [GELF](https://github.com/log4js-node/gelf)
* [Loggly](https://github.com/log4js-node/loggly)
* Logstash ([UDP](https://github.com/log4js-node/logstashUDP) and [HTTP](https://github.com/log4js-node/logstashHTTP))
* logFaces ([UDP](https://github.com/log4js-node/logFaces-UDP) and [HTTP](https://github.com/log4js-node/logFaces-HTTP))
* [RabbitMQ](https://github.com/log4js-node/rabbitmq)
* [Redis](https://github.com/log4js-node/redis)
* [Hipchat](https://github.com/log4js-node/hipchat)
* [Slack](https://github.com/log4js-node/slack)
* [mailgun](https://github.com/log4js-node/mailgun)


## Getting help
Having problems? Jump on the [slack](https://join.slack.com/t/log4js-node/shared_invite/enQtMjk5OTcxODMwNDA1LTk5ZTA0YjcwNWRiYmFkNGQyZTkyZTYzYTFiYTE2NTRhNzFmNmY3OTdjZTY3MWM3M2RlMGQxN2ZlMmY4ZDFmZWY) channel, or create an issue. If you want to help out with the development, the slack channel is a good place to go as well.

## installation

```bash
npm install log4js
```

## usage

Minimalist version:
```javascript
var log4js = require('log4js');
var logger = log4js.getLogger();
logger.level = 'debug';
logger.debug("Some debug messages");
```
By default, log4js will not output any logs (so that it can safely be used in libraries). The `level` for the `default` category is set to `OFF`. To enable logs, set the level (as in the example). This will then output to stdout with the coloured layout (thanks to [masylum](http://github.com/masylum)), so for the above you would see:
```bash
[2010-01-17 11:43:37.987] [DEBUG] [default] - Some debug messages
```
See example.js for a full example, but here's a snippet (also in `examples/fromreadme.js`):
```javascript
const log4js = require('log4js');
log4js.configure({
  appenders: { cheese: { type: 'file', filename: 'cheese.log' } },
  categories: { default: { appenders: ['cheese'], level: 'error' } }
});

const logger = log4js.getLogger('cheese');
logger.trace('Entering cheese testing');
logger.debug('Got cheese.');
logger.info('Cheese is Comté.');
logger.warn('Cheese is quite smelly.');
logger.error('Cheese is too ripe!');
logger.fatal('Cheese was breeding ground for listeria.');
```
Output (in `cheese.log`):
```bash
[2010-01-17 11:43:37.987] [ERROR] cheese - Cheese is too ripe!
[2010-01-17 11:43:37.990] [FATAL] cheese - Cheese was breeding ground for listeria.
```

## Note for library makers

If you're writing a library and would like to include support for log4js, without introducing a dependency headache for your users, take a look at [log4js-api](https://github.com/log4js-node/log4js-api).

## Documentation
Available [here](https://log4js-node.github.io/log4js-node/).

There's also [an example application](https://github.com/log4js-node/log4js-example).

## TypeScript
```ts
import { configure, getLogger } from './log4js';
configure('./filename');
const logger = getLogger();
logger.level = 'debug';
logger.debug("Some debug messages");

configure({
	appenders: { cheese: { type: 'file', filename: 'cheese.log' } },
	categories: { default: { appenders: ['cheese'], level: 'error' } }
});
```

## Contributing

We're always looking for people to help out. Jump on [slack](https://join.slack.com/t/log4js-node/shared_invite/enQtMjk5OTcxODMwNDA1LTk5ZTA0YjcwNWRiYmFkNGQyZTkyZTYzYTFiYTE2NTRhNzFmNmY3OTdjZTY3MWM3M2RlMGQxN2ZlMmY4ZDFmZWY) and discuss what you want to do. Also, take a look at the [rules](https://log4js-node.github.io/log4js-node/contrib-guidelines.html) before submitting a pull request.

## License

The original log4js was distributed under the Apache 2.0 License, and so is this. I've tried to
keep the original copyright and author credits in place, except in sections that I have rewritten
extensively.
