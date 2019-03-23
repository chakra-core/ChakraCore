# silent-error [![Build Status](https://travis-ci.org/ember-cli/silent-error.svg)](https://travis-ci.org/ember-cli/silent-error)

An error subclass for humanized errors. This module allows for inter-module detection of errors which are fatal, but where a stacktrace by default provides negative value.

Some use-cases:

* command in your CLI tool is missing
* plugin to your build system is given invalid user-input.

Obviously stack traces can still be valuable. To view the stacks, the following environment variable can be set to `true`

```
SILENT_ERROR=verbose <run program>
```

## Example

```js
// in one node module
async function runCommand(name) {
   // some logic
   throw new SilentError(`command: '${name}' is not installed`);
}
```

```js
// in another node_module
async function caller() {

  try {
    await runCommand('foo');
  } catch(e) {
    SilentError.debugOrThrow(e);
  }

  SilentError.debugOrThrow
}
```

## Installation

```
yarn add silent-error
```

or

```
npm install --save silent-error
```
