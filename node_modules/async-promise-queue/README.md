# async-promise-queue

[![Build Status](https://travis-ci.org/stefanpenner/async-promise-queue.svg?branch=master)](https://travis-ci.org/stefanpenner/async-promise-queue)

A wrapper around the `async` module, that provides an improved promise queue.

Some highlights:

* promiseified method (all wired up)
* in a failure scenario, it will wait for pending work before rejecting. This prevents the run-away work problem.

## Usage

```sh
npm install async-promise-queue
```

or

```sh
yarn add async-promise-queue
```

### Debug logging

```
DEBUG="async-promise-queue*" node <your program>
```

And you will be informed when a queue is used, and what its concurrency becomes (note: we can always add more logging, submit your ideas as pull requests!)

## Example

```js
'use strict';

const queue = require('async-promise-queue');

queue.async // a reference to the `async` module which `async-promise-queue` is requiring.

// the example worker
const worker = queue.async.asyncify(function(work) {
  console.log('work', work.file);
  return new Promise(resolve => {
    if (work.file === '/path-2') { throw new Error('/path-2'); }
    if (work.file === '/path-3') { throw new Error('/path-3'); }
    setTimeout(resolve, work.duration);
  });
});

// the work
const work = [
  { file:'/path-1',  duration: 1000 },
  { file:'/path-2',  duration: 50 },
  { file:'/path-3',  duration: 100 },
  { file:'/path-4',  duration: 50 },
];

// calling our queue helper
queue(worker, work, 3)
  .catch(reason => console.error(reason))
  .then(value   => console.log('complete!!', value))
```
