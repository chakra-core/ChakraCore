# console-ui

[![Build Status](https://travis-ci.org/ember-cli/console-ui.svg?branch=master)](https://travis-ci.org/ember-cli/console-ui)
[![Build status](https://ci.appveyor.com/api/projects/status/38tkats2frmmxt2f/branch/master?svg=true)](https://ci.appveyor.com/project/embercli/console-ui/branch/master)

The goal of this library is to declare a common interface that various
node_modules can utilize allowing their UI interactions to be well
coordinated and interoperable. This repo provides reference UI
and test mock implementations. In theory, an alternate implementation abiding
by the describe API could be provided, and the system and all participating
libraries would continue to function correctly.

Features:

* unified and pluggable input/output streams for all participants
* system wide writeLevels enabling ability to easily silence/warn/debug print
  while abiding by shared configuration
* unified progress
* unified CI state (to disable CI unfriendly features such as progress spinners)
* simple
* test mock

## Usage

```js
const UI = require('console-ui')
const ui = new UI({
  inputStream: process.stdin,
  outputStream: process.stdout,
  errorStream: process.stderr,
  writeLevel: 'DEBUG' | 'INFO' | 'WARNING' | 'ERROR',
  ci: true | false
});
```

write to output:

```js
ui.write('message');
ui.write('message', 'ERROR'); // specify  writelevel
```


write + newline to output:

```js
ui.writeLine('message');
ui.writeLine('message', 'ERROR'); // specify  writelevel
```

write with DEBUG writeLevel

```js
ui.writeDebugLine('message');
```

write with INFO writeLevel

```js
ui.writeInfoLine('message');
```

write with WARN writeLevel

```js
ui.writeWarnLine('message');
```

write a message related to a deprecation

```js
ui.writeDeprecateLine('some message', true | false); // pass boolean as second argument indicating if deprecated or not
```

write an error nicely (in red) to the console:

* if error.file || error.filename, nicely print the file name
  * if error.line also nicely print file + line
  * if error.col also nicely print file + line + col

* if error.message, nicely print it
* if error.stack, nicely print it

```js
ui.writeError(error);
```

to adjust the writeLevel on the fly:

```js
ui.setWriteLevel('DEBUG' || 'INFO' || 'WARNING' || 'ERROR');
```


to begin progress spinner \w message (only if INFO writeLevel is visible)

```js
ui.startProgress('building...');
```

to end progress spinner

```js
ui.stopProgress();
```

to prompt a user, via [inquirer](https://www.npmjs.com/package/inquirer)

```js
ui.prompt(queryForInquirer, callback);
```

to query if a given `writeLevel` is visible:

```js
ui.writeLevelVisible('DEBUG' || 'INFO' || 'WARNING' || 'ERROR'); // => true | false
```
