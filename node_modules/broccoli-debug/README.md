# broccoli-debug [![Build Status](https://travis-ci.org/broccolijs/broccoli-debug.svg?branch=master)](https://travis-ci.org/broccolijs/broccoli-debug) [![Build status](https://ci.appveyor.com/api/projects/status/u6tkot7ru19wntxr/branch/master?svg=true)](https://ci.appveyor.com/project/embercli/broccoli-debug/branch/master)


Utility for build pipeline authors to allow trivial debugging of the Broccoli
pipelines they author.

Heavily inspired by [@stefanpenner](https://github.com/stefanpenner)'s
[broccoli-stew's `debug`](https://github.com/stefanpenner/broccoli-stew/blob/v1.4.2/lib/debug.js)'s helper,
but improved in a few ways:

* Supports leaving debug trees in the build with minimal cost when not being used.
* Supports binary files (e.g. does not write `.png`'s as `utf8` text).
* Adds [debug](https://github.com/visionmedia/debug) style debug matching.

## Usage

### Pipeline Authors

To allow consumers to debug the internals of various stages in your build pipeline,
you create a new instance of `BroccoliDebug` and return it instead.

Something like this:

```js
var BroccoliDebug = require('broccoli-debug');

let tree = new BroccoliDebug(input, `ember-engines:${this.name}:addon-input`);
```

Obviously, this would get quite verbose to do many times, so we have created a shortcut
to easily create a number of debug trees with a shared prefix:

```js
let debugTree = BroccoliDebug.buildDebugCallback(`ember-engines:${this.name}`);

let tree1 = debugTree(input1, 'addon-input');
// tree1.debugLabel => 'ember-engines:<some-name>:addon-input'

let tree2 = debugTree(input2, 'addon-output');
// tree2.debugLabel => 'ember-engines:<some-name>:addon-output
```

### Consumers

Folks wanting to inspect the state of the build pipeline at that stage, would do the following:

```js
BROCCOLI_DEBUG=ember-engines:* ember b
```

Now you can take a look at the state of that input tree by:

```js
ls DEBUG/ember-engines/*
```

### API

```ts
interface BroccoliDebugOptions {
  /**
    The label to use for the debug folder. By default, will be placed in `DEBUG/*`.
  */
  label: string

  /**
    The base directory to place the input node contents when debugging is enabled.

    Chooses the default in this order:

    * `process.env.BROCCOLI_DEBUG_PATH`
    * `path.join(process.cwd(), 'DEBUG')`
  */
  baseDir: string

  /**
    Should the tree be "always on" for debugging? This is akin to `debugger`, its very
    useful while actively working on a build pipeline, but is likely something you would
    remove before publishing.
  */
  force?: boolean
}

class BroccoliDebug {
  /**
    Builds a callback function for easily generating `BroccoliDebug` instances
    with a shared prefix.
  */
  static buildDebugCallback(prefix: string): (node: any, labelOrOptions: string | BroccoliDebugOptions) => BroccoliNode
  constructor(node: BroccoliNode, labelOrOptions: string | BroccoliDebugOptions);
  debugLabel: string;
}
```

## Development

### Installation

* `git clone git@github.com:broccolijs/broccoli-debug.git`
* `cd broccoli-debug`
* `yarn`

### Testing

* `yarn test`
