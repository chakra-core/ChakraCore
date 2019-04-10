# broccoli-babel-transpiler

[![Build Status](https://travis-ci.org/babel/broccoli-babel-transpiler.svg?branch=master)](https://travis-ci.org/babel/broccoli-babel-transpiler)
[![Build status](https://ci.appveyor.com/api/projects/status/a0nbd84m1x4y5fp5?svg=true)](https://ci.appveyor.com/project/embercli/broccoli-babel-transpiler)


A [Broccoli](https://github.com/broccolijs/broccoli) plugin which transpiles ES6 to readable ES5 (and much more) by using [Babel](https://github.com/babel/babel).

## How to install?

```sh
$ npm install broccoli-babel-transpiler --save-dev
```

## How to use?

In your `Brocfile.js`:

```js
const babel = require('broccoli-babel-transpiler');
const scriptTree = babel(inputTree, babelOptions);
```

Note that, since Babel 6 (and v6 of this plugin), you need to be specific as to
what your transpilation target is. Running `esTranspiler` with empty options
will not transpile anything. You will need:

  * Explicit options, such as `presets`. See available [options](https://babeljs.io/docs/usage/options) at Babel's GitHub repo.
  * Babel plugins that implement the transforms you require.

For a quick running example, install this plugin:

```sh
$ npm install babel-preset-env
```

And then run the transform like this:

```js
const babel = require('broccoli-babel-transpiler');
let scriptTree = babel(inputTree, {
  presets: [
    ['env', {
      'targets': {
        'browsers': ['last 2 versions']
      }
    }]
  ]
});
```

### Examples

You'll find three example projects using this plugin in the repository
[broccoli-babel-examples](https://github.com/givanse/broccoli-babel-examples).
Each one of them builds on top of the previous example so you can progess from
bare minimum to ambitious development.

 * [es6-fruits](https://github.com/givanse/broccoli-babel-examples/tree/master/es6-fruits) - Execute a single ES6 script.
 * [es6-website](https://github.com/givanse/broccoli-babel-examples/tree/master/es6-website) - Build a simple website.
 * [es6-modules](https://github.com/givanse/broccoli-babel-examples/tree/master/es6-modules) - Handle modules and unit tests.

## About source map

Currently this plugin only supports inline source map. If you need
separate source map feature, you're welcome to submit a pull request.

## Advanced usage

`filterExtensions` is an option to limit (or expand) the set of file extensions
that will be transformed.

The default `filterExtension` is `js`

```js
const esTranspiler = require('broccoli-babel-transpiler');
let scriptTree = esTranspiler(inputTree, {
    filterExtensions:['js', 'es6'] // babelize both .js and .es6 files
});
```

`targetExtension` is an option to specify the extension of the output files

The default `targetExtension` is `js`

```js
const esTranspiler = require('broccoli-babel-transpiler');
let scriptTree = esTranspiler(inputTree, {
    targetExtension: 'module.js' // create output files with module.js extension
});
```

## Polyfill

In order to use some of the ES6 features you must include the Babel
[polyfill](http://babeljs.io/docs/usage/polyfill/#usage-in-browser).

You don't always need this, review which features need the polyfill here: [ES6
Features](https://babeljs.io/docs/learn-es6).

```js
const esTranspiler = require('broccoli-babel-transpiler');
let scriptTree = esTranspiler(inputTree, { browserPolyfill: true });
```

By default, the polyfill will appear as `./browser-polyfill.js` in the output
tree. You can specify a custom output path and filename by supplying a (relative)
filename in the `browserPolyfillPath` option:

```js
let scriptTree = esTranspiler(inputTree, {
  browserPolyfill: true,
  browserPolyfillPath: 'vendor/polyfill.js'
})
```

## Plugins

Use of custom plugins works similarly to `babel` itself. You would pass a
`plugins` array in `options`:

```js
const esTranspiler = require('broccoli-babel-transpiler');
const applyFeatureFlags = require('babel-plugin-feature-flags');

let featureFlagPlugin = applyFeatureFlags({
  import: { module: 'ember-metal/features' },
  features: {
    'ember-metal-blah': true
  }
});

let scriptTree = esTranspiler(inputTree, {
  plugins: [
    featureFlagPlugin
  ]
});
```

### Caching

broccoli-babel-transpiler uses a persistent cache to enable rebuilds to be
significantly faster (by avoiding transpilation for files that have not
changed). However, since a plugin can do many things to affect the transpiled
output it must also influence the cache key to ensure transpiled files are
rebuilt
if the plugin changes (or the plugins configuration).

In order to aid plugin developers in this process, broccoli-babel-transpiler
will invoke two methods on a plugin so that it can augment the cache key:

* `cacheKey` - This method is used to describe any runtime information that may
  want to invalidate the cached result of each file transpilation. This is
  generally only needed when the configuration provided to the plugin is used
  to modify the AST output by a plugin like `babel-plugin-filter-imports`
  (module
  exports to strip from a build), `babel-plugin-feature-flags` (configured
  features and current status to strip or embed in a final build), or
  `babel-plugin-htmlbars-inline-precompile` (uses `ember-template-compiler.js`
  to compile inlined templates).
* `baseDir` - This method is expected to return the plugins base dir. The
  provided `baseDir` is used to ensure the cache is invalidated if any of the
  plugin's files change (including its deps). Each plugin should implement
  `baseDir` as: `Plugin.prototype.baseDir = function() { return \_\_dirname;
  };`.

## Parallel Transpilation

broccoli-babel-transpiler can run multiple babel transpiles in parallel using a
pool of workers, to take advantage of multi-core systems. Because these workers
are separate processes, the plugins and callback functions that are normally
passed as options to babel must be specified in a serializable form.
To enable this parallelization there is an API to tell the worker how to
construct the plugin or callback in its process.

To ensure a build remains parallel safe, one can set the
`throwUnlessParallelizable` option to true (defaults to false). This will cause
an error to be thrown, if parallelization is not possible due to an
incompatible babel plugin.

```js
new Babel(input, { throwUnlessParallelizable: true | false });
```

Alternatively, an environment variable can be set:

```sh
THROW_UNLESS_PARALLELIZABLE=1 node build.js
```

Plugins are specified as an object with a `_parallelBabel` property:

```js
let plugin = {
  _parallelBabel: {
    requireFile: '/full/path/to/the/file',
    useMethod: 'methodName',
    buildUsing: 'buildFunction',
    params: { ok: 'this object will be passed to buildFunction()' }
  }
};
```

Callbacks can be specified like plugins, or as functions with a
`_parallelBabel` property:

```js
function callback() { /* do something */ };

callback._parallelBabel = {
  requireFile: '/full/path/to/the/file',
  useMethod: 'methodName',
  buildUsing: 'buildFunction',
  params: { ok: 'this object will be passed to buildFunction()' }
};
```

### requireFile (required)

This property specifies the file to require in the worker process to create the
plugin or callback. This must be given as an absolute path.

```js
const esTranspiler = require('broccoli-babel-transpiler');

let somePlugin = {
  _parallelBabel: {
    requireFile: '/full/path/to/the/file'
  }
});

let scriptTree = esTranspiler(inputTree, {
  plugins: [
    'transform-strict-mode', // plugins that are given as strings will automatically be parallelized
    somePlugin
  ]
});
```

### useMethod (optional)

This property specifies the method to use from the file that is required.

If you have a plugin defined like this:

```js
// some_plugin.js

module.exports = {
  pluginFunction(babel) {
    // do plugin things
  }
};
```

You can tell broccoli-babel-transpiler to use that function in the worker
processes like so:

```js
const esTranspiler = require('broccoli-babel-transpiler');

let somePlugin = {
  _parallelBabel: {
    requireFile: '/path/to/some_plugin',
    useMethod: 'pluginFunction'
  }
});

let scriptTree = esTranspiler(inputTree, {
  plugins: [ somePlugin ]
});
```

### buildUsing and params (optional)

These properties specify a function to run to build the plugin (or callback),
and any parameters to pass to that function.

If the plugin needs to be built dynamically, you can do that like so:

```js
// some_plugin.js

module.exports = {
  buildPlugin(params) {
    return doSomethingWith(params.text);
  }
};
```

This will tell the worker process to require the plugin and call the
`buildPlugin` function with the `params` object as an argument:

```js
const esTranspiler = require('broccoli-babel-transpiler');

let somePlugin = {
  _parallelBabel: {
    requireFile: '/path/to/some_plugin',
    buildUsing: 'buildPlugin',
    params: { text: 'some text' }
  }
});

let scriptTree = esTranspiler(inputTree, {
  plugins: [ somePlugin ]
});
```

Note: If both `useMethod` and `buildUsing` are specified, `useMethod` takes
precedence.

### Number of jobs

The number of parallel jobs defaults to the number of detected CPUs - 1.

This can be changed with the `JOBS` environment variable:

```sh
JOBS=4 ember build
```

To disable parallelization:

```sh
JOBS=1 ember build
```
