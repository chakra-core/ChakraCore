async-settle
============

[![build status](https://secure.travis-ci.org/phated/async-settle.png)](http://travis-ci.org/phated/async-settle)

Settle your async functions - when you need to know all your parallel functions are complete (success or failure)

## API

### `settle(executor, onComplete)` : Function

Takes a function to execute (`executor`) and a function to call on completion (`onComplete`).

`executer` is executed in the context of [`async-done`](https://github.com/phated/async-done), with all errors and results being settled.

`onComplete` will be called with a settled value.

#### Settled Values

Settled values have two properties, `state` and `value`.

`state` has two possible options `'error'` and `'success'`.

`value` will be the value passed to original callback.

## License

MIT
