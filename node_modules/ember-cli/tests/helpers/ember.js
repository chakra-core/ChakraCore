'use strict';

const MockUI = require('console-ui/mock');
const MockAnalytics = require('./mock-analytics');
const cli = require('../../lib/cli');
const MockProcess = require('./mock-process');
const path = require('path');

/*
  Accepts a single array argument, that contains the
  `process.argv` style arguments.

  Example:

  ```
  ember test --no-build --test-port=4567
  ```

  In this example, `process.argv` would be something similar to:

  ```
  ['path/to/node', 'path/to/bin/ember', 'test', '--no-build', '--test-port=4567']
  ```


  And this could be emulated by calling:

  ```
  ember(['test', '--no-build', '--test-port=4567']);
  ```

  ---

  The return value of this helper is a promise that resolves
  with an object that contains the following properties:

  * `exitCode` is the normal exit code in standard command invocation
  * `ui` is the `ui` object that was used for the invocation (which is
    a `MockUI` instance), this can be used to inspect the commands output.

*/
module.exports = function ember(args, options) {
  let cliInstance;
  let ui = (options && options.UI) || MockUI;
  let pkg = (options && options.package) || path.resolve(__dirname, '..', '..');
  let disableDependencyChecker = (options && options.disableDependencyChecker) || true;
  let inputStream = [];
  let outputStream = [];
  let errorLog = [];
  let commandName = args[0];

  if (commandName === 'test') {
    /*
      we are forced to invalidate testem config module cache
      to ensure that each test always reads from the file system,
      not the memory cache.
    */
    delete require.cache[path.join(process.cwd(), 'testem.js')];
  }

  args.push('--disable-analytics');
  args.push('--watcher=node');

  if (!options || options.skipGit !== false) {
    args.push('--skipGit');
  }

  cliInstance = cli({
    process: new MockProcess(),
    inputStream,
    outputStream,
    errorLog,
    cliArgs: args,
    Leek: MockAnalytics,
    UI: ui,
    testing: true,
    disableDependencyChecker,
    cli: {
      // This prevents ember-cli from detecting any other package.json files
      // forcing ember-cli to act as the globally installed package
      npmPackage: 'ember-cli',
      root: pkg,
    },
  });

  function returnTestState(statusCode) {
    let result = {
      exitCode: statusCode,
      statusCode,
      inputStream,
      outputStream,
      errorLog,
    };

    if (statusCode) {
      throw result;
    } else {
      return result;
    }
  }

  return cliInstance.then(returnTestState);
};
