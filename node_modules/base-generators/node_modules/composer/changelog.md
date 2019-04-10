### v0.13.0

- Skip tasks by setting the `options.skip` option to the name of the task or an array of task names.
- Making additional `err` properties non-enumerable to cut down on error output.

### v0.12.0

- You can no longer get a task from the `.task()` method by passing only the name. Instead do `var task = app.tasks[name];`
- Passing only a name and no dependencies to `.task()` will result in a `noop` task being created.
- `options` may be passed to `.build()`, `.series()` and `.parallel()`
- `options` passed to `.build()` will be merged onto task options before running the task.
- Skip tasks by setting their `options.run` option to `false`.

### v0.11.3

- Allow passing es2015 javascript generator functions to `.task()`.

### v0.11.2

- Allow using glob patterns for task dependencies.

### v0.11.0

- **BREAKING CHANGE**: Removed `.watch()`. Watch functionality can be added to [base][] applications using [base-watch][].

### v0.10.0

- Removes `session`.

### v0.9.0

- Use `default` when no tasks are passed to `.build()`.

### v0.8.4

- Ensure task dependencies are unique.

### v0.8.2

- Emitting `task` when adding a task through `.task()`
- Returning task when calling `.task(name)` with only a name.

### v0.8.0

- Emitting `task:*` events instead of generic `*` events. See [event docs](#events) for more information.

### v0.7.0

- No longer returning the current task when `.task()` is called without a name.
- Throwing an error when `.task()` is called without a name.

### v0.6.0

- Adding properties to `err` instances and emitting instead of emitting multiple parameters.
- Adding series and parallel flows/methods.

### v0.5.0

- **BREAKING CHANGE** Renamed `.run()` to `.build()`

### v0.4.2

- `.watch` returns an instance of `FSWatcher`

### v0.4.1

- Currently running task returned when calling `.task()` without a name.

### v0.4.0

- Add session-cache to enable per-task data contexts.

### v0.3.0

- Event bubbling/emitting changed.

### v0.1.0

- Initial release.
