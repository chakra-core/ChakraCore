This directory contains the WebAssembly test suite. It is split into two
directories:

* [`core/`](core/), tests for the core semantics
* [`js-api/`](js-api/), tests for the JavaScript API.
* [`html/`](html/), tests for the JavaScript API in a DOM environment.

A [landing page](out/index.html) contains a condensed version made of all
these tests, converted to HTML.

A list of to-do's can be found [here](Todo.md).

## Multi-stage testing

The wast tests can be converted to JavaScript, and the JavaScript tests
to HTML tests, using the `build.py` script. It will create a `out/` directory
(checked in in this repository, to be able to use it from github pages),
containing subdirectories with expanded tests, as well as a landing page for
runnning all of them in HTML.

The HTML tests are just [Web Platform Tests](http://testthewebforward.org)
using the
[testharness.js](http://testthewebforward.org/docs/testharness-library.html)
library.

Each wast test gets its equivalent JS test, and each JS test (including wast
test) gets its equivalent WPT, to be easily run in browser vendors' automation.

## Procedure for adding a new test

- put the test in the right directory according to the above (top) description.
- ideally, commit here so the actual content commit and build commit are
  separated.
- re-run `build.py` so that the landing page is updated and all the cascading
  happens.
- re-commit here, if necessary.

## Local HTTP serving of the repository

From the root of your clone of this repository:

```
python -m SimpleHTTPServer 8000
```

Then open your favorite browser and browse to `http://localhost:8000/test/out`.
