This directory contains tests specific to the [JavaScript API] to WebAssembly.

These tests exist in the [web-platform-tests project], and are included here
primarily to simplify sharing them between JavaScript engine implementers.

These tests can be run in a pure JavaScript environment, that is, a JS shell
(like V8 or spidermonkey's shells), as well as in the browser.

The tests use the [testharness.js] library and the [multi-global tests] setup
for smooth integrations with the web-platform-tests project and the existing
test infrastructure in implementations.

## Metadata

All tests must have the `.any.js` extension.

In order to be run in the JavaScript shell, a metadata comment to add the
`jsshell` scope to the default set of scopes (`window` and `dedicatedworker`)
is required at the start of the file:

```js
// META: global=jsshell
```

Additional JavaScript files can be imported with

```js
// META: script=helper-file.js

```

## Harness

A single test file contains multiple subtests, which are created with one of
the following functions:

- For synchronous tests: `test(function, name)` runs the function immediately.
  The test fails if any assertion fails or an exception is thrown while calling
  the function.
- For asynchronous tests: `promise_test(function, name)` where `function`
  returns a `Promise`. The test fails if the returned `Promise` rejects.

All assertions must be in one of those subtests.

A number of assertion functions are provided, e.g.:

- `assert_equals(x, y)`;
- `assert_not_equals(x, y)`;
- `assert_true(x)`;
- `assert_false(x)`;
- `assert_unreached()`;
- `assert_throws(error, function)`: checks if `function` throws an appropriate
  exception (a typical value for `error` would be `new TypeError()`);
- `assert_class_string(object, class_name)`: checks if the result of calling
  `Object.prototype.toString` on `object` uses `class_name`;
- `promise_rejects`: `assert_throws`, but for `Promise`s.

All the above functions also take an optional trailing description argument.

Non-trivial code that runs before the subtests should be put in the function
argument to `setup(function)`, to ensure any exceptions are handled gracefully.

Finally, the harness also exposes a `format_value` function that provides a
helpful stringification of its argument.

See the [testharness.js] documentation for more information.

[JavaScript API]: https://webassembly.github.io/spec/js-api/
[web-platform-tests project]: https://github.com/web-platform-tests/wpt/tree/master/wasm/jsapi
[testharness.js]: https://web-platform-tests.org/writing-tests/testharness-api.html
[multi-global tests]: https://web-platform-tests.org/writing-tests/testharness.html#multi-global-tests
