# composer [![NPM version](https://img.shields.io/npm/v/composer.svg?style=flat)](https://www.npmjs.com/package/composer) [![NPM downloads](https://img.shields.io/npm/dm/composer.svg?style=flat)](https://npmjs.org/package/composer) [![Build Status](https://img.shields.io/travis/doowb/composer.svg?style=flat)](https://travis-ci.org/doowb/composer)

API-first task runner with three methods: task, run and watch.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install composer --save
```

**Heads up** the `.watch` method was removed in version `0.11.0`. If you need _watch_ functionality, use [base-tasks](https://github.com/jonschlinkert/base-tasks) and [base-watch](https://github.com/node-base/base-watch).

## Usage

```js
var Composer = require('composer');
```

## API

### [.task](index.js#L62)

Register a new task with it's options and dependencies.

Dependencies may also be specified as a glob pattern. Be aware that
the order cannot be guarenteed when using a glob pattern.

**Params**

* `name` **{String}**: Name of the task to register
* `options` **{Object}**: Options to set dependencies or control flow.
* `options.deps` **{Object}**: array of dependencies
* `options.flow` **{Object}**: How this task will be executed with it's dependencies (`series`, `parallel`, `settleSeries`, `settleParallel`)
* `deps` **{String|Array|Function}**: Additional dependencies for this task.
* `fn` **{Function}**: Final function is the task to register.
* `returns` **{Object}**: Return the instance for chaining

**Example**

```js
// register task "site" with composer
app.task('site', ['styles'], function() {
  return app.src('templates/pages/*.hbs')
    .pipe(app.dest('_gh_pages'));
});
```

### [.build](index.js#L118)

Build a task or array of tasks.

**Params**

* `tasks` **{String|Array|Function}**: List of tasks by name, function, or array of names/functions. (Defaults to `[default]`).
* `options` **{Object}**: Optional options object to merge onto each task's options when building.
* `cb` **{Function}**: Callback function to be called when all tasks are finished building.

**Example**

```js
app.build('default', function(err, results) {
  if (err) return console.error(err);
  console.log(results);
});
```

### [.series](index.js#L185)

Compose task or list of tasks into a single function that runs the tasks in series.

**Params**

* `tasks` **{String|Array|Function}**: List of tasks by name, function, or array of names/functions.
* `returns` **{Function}**: Composed function that may take a callback function.

**Example**

```js
app.task('foo', function(done) {
  console.log('this is foo');
  done();
});

var fn = app.series('foo', function bar(done) {
  console.log('this is bar');
  done();
});

fn(function(err) {
  if (err) return console.error(err);
  console.log('done');
});
//=> this is foo
//=> this is bar
//=> done
```

### [.parallel](index.js#L217)

Compose task or list of tasks into a single function that runs the tasks in parallel.

**Params**

* `tasks` **{String|Array|Function}**: List of tasks by name, function, or array of names/functions.
* `returns` **{Function}**: Composed function that may take a callback function.

**Example**

```js
app.task('foo', function(done) {
  setTimeout(function() {
    console.log('this is foo');
    done();
  }, 500);
});

var fn = app.parallel('foo', function bar(done) {
  console.log('this is bar');
  done();
});

fn(function(err) {
  if (err) return console.error(err);
  console.log('done');
});
//=> this is bar
//=> this is foo
//=> done
```

## Events

[composer](https://github.com/doowb/composer) is an event emitter that may emit the following events:

### starting

This event is emitted when a `build` is starting.

The event emits 2 arguments, the current instance of [composer](https://github.com/doowb/composer) as the `app` and an object containing the build runtime information.

```js
app.on('starting', function(app, build) {});
```

* `build` exposes a `.date` object that has a `.start` property containing the start time as a `Date` object.
* `build` exposes a `.hr` object that has a `.start` property containing the start time as an `hrtime` array.

### finished

This event is emitted when a `build` has finished.

The event emits 2 arguments, the current instance of [composer](https://github.com/doowb/composer) as the `app` and an object containing the build runtime information.

```js
app.on('finished', function(app, build) {});
```

* `build` exposes a `.date` object that has `.start` and `.end` properties containing start and end times of the build as `Date` objects.
* `build` exposes a `.hr` object that has `.start`, `.end`, `.duration`, and `.diff` properties containing timing information calculated using `process.hrtime`

### error

This event is emitted when an error occurrs during a `build`.
The event emits 1 argument as an `Error` object containing additional information about the build and the task running when the error occurred.

```js
app.on('error', function(err) {});
```

Additional properties:

* `app`: current composer instance running the build
* `build`: current build runtime information
* `task`: current task instance running when the error occurred
* `run`: current task runtime information

### task:starting

This event is emitted when a task is starting.
The event emits 2 arguments, the current instance of the task object and an object containing the task runtime information.

```js
app.on('task:starting', function(task, run) {});
```

The `run` parameter exposes:

* `.date` **{Object}**: has a `.start` property containing the start time as a `Date` object.
* `.hr` **{Object}**: has a `.start` property containing the start time as an `hrtime` array.

### task:finished

This event is emitted when a task has finished.

The event emits 2 arguments, the current instance of the task object and an object containing the task runtime information.

```js
app.on('task:finished', function(task, run) {});
```

The `run` parameter exposes:

* `.date` **{Object}**: has a `.date` object that has `.start` and `.end` properties containing start and end times of the task as `Date` objects.
* `run` **{Object}**: has an `.hr` object that has `.start`, `.end`, `.duration`, and `.diff` properties containing timing information calculated using `process.hrtime`

### task:error

This event is emitted when an error occurrs while running a task.
The event emits 1 argument as an `Error` object containing additional information about the task running when the error occurred.

```js
app.on('task:error', function(err) {});
```

**Additional properties**

* `task`: current task instance running when the error occurred
* `run`: current task runtime information

## History

### v0.13.0

* Skip tasks by setting the `options.skip` option to the name of the task or an array of task names.
* Making additional `err` properties non-enumerable to cut down on error output.

### v0.12.0

* You can no longer get a task from the `.task()` method by passing only the name. Instead do `var task = app.tasks[name];`
* Passing only a name and no dependencies to `.task()` will result in a `noop` task being created.
* `options` may be passed to `.build()`, `.series()` and `.parallel()`
* `options` passed to `.build()` will be merged onto task options before running the task.
* Skip tasks by setting their `options.run` option to `false`.

### v0.11.3

* Allow passing es2015 javascript generator functions to `.task()`.

### v0.11.2

* Allow using glob patterns for task dependencies.

### v0.11.0

* **BREAKING CHANGE**: Removed `.watch()`. Watch functionality can be added to [base](https://github.com/node-base/base) applications using [base-watch](https://github.com/node-base/base-watch).

### v0.10.0

* Removes `session`.

### v0.9.0

* Use `default` when no tasks are passed to `.build()`.

### v0.8.4

* Ensure task dependencies are unique.

### v0.8.2

* Emitting `task` when adding a task through `.task()`
* Returning task when calling `.task(name)` with only a name.

### v0.8.0

* Emitting `task:*` events instead of generic `*` events. See [event docs](#events) for more information.

### v0.7.0

* No longer returning the current task when `.task()` is called without a name.
* Throwing an error when `.task()` is called without a name.

### v0.6.0

* Adding properties to `err` instances and emitting instead of emitting multiple parameters.
* Adding series and parallel flows/methods.

### v0.5.0

* **BREAKING CHANGE** Renamed `.run()` to `.build()`

### v0.4.2

* `.watch` returns an instance of `FSWatcher`

### v0.4.1

* Currently running task returned when calling `.task()` without a name.

### v0.4.0

* Add session-cache to enable per-task data contexts.

### v0.3.0

* Event bubbling/emitting changed.

### v0.1.0

* Initial release.

## Related projects

You might also be interested in these projects:

* [assemble](https://www.npmjs.com/package/assemble): Assemble is a powerful, extendable and easy to use static site generator for node.js. Used… [more](https://www.npmjs.com/package/assemble) | [homepage](https://github.com/assemble/assemble)
* [base-tasks](https://www.npmjs.com/package/base-tasks): base-methods plugin that provides a very thin wrapper around [https://github.com/jonschlinkert/composer](https://github.com/jonschlinkert/composer) for adding task methods to… [more](https://www.npmjs.com/package/base-tasks) | [homepage](https://github.com/jonschlinkert/base-tasks)
* [generate](https://www.npmjs.com/package/generate): Fast, composable, highly extendable project generator with a user-friendly and expressive API. | [homepage](https://github.com/generate/generate)
* [update](https://www.npmjs.com/package/update): Easily keep anything in your project up-to-date by installing the updaters you want to use… [more](https://www.npmjs.com/package/update) | [homepage](https://github.com/update/update)
* [verb](https://www.npmjs.com/package/verb): Documentation generator for GitHub projects. Verb is extremely powerful, easy to use, and is used… [more](https://www.npmjs.com/package/verb) | [homepage](https://github.com/verbose/verb)

## Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](https://github.com/doowb/composer/issues/new).

## Building docs

Generate readme and API documentation with [verb](https://github.com/verbose/verb):

```sh
$ npm install verb && npm run docs
```

Or, if [verb](https://github.com/verbose/verb) is installed globally:

```sh
$ verb
```

## Running tests

Install dev dependencies:

```sh
$ npm install -d && npm test
```

## Author

**Jon Schlinkert**

* [github/jonschlinkert](https://github.com/jonschlinkert)
* [twitter/jonschlinkert](http://twitter.com/jonschlinkert)

## License

Copyright © 2016, [Jon Schlinkert](https://github.com/jonschlinkert).
Released under the [MIT license](https://github.com/doowb/composer/blob/master/LICENSE).

***

_This file was generated by [verb](https://github.com/verbose/verb), v0.9.0, on May 25, 2016._