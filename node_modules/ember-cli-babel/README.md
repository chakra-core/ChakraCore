# ember-cli-babel

[![Build Status](https://travis-ci.org/babel/ember-cli-babel.svg?branch=master)](https://travis-ci.org/babel/ember-cli-babel)
[![Build status](https://ci.appveyor.com/api/projects/status/2a6pspve1wrwwyj5/branch/master?svg=true)](https://ci.appveyor.com/project/embercli/ember-cli-babel/branch/master)


This Ember-CLI plugin uses [Babel](https://babeljs.io/) and
[babel-preset-env](https://github.com/babel/babel-preset-env) to allow you to
use ES6 syntax with your Ember CLI project.

## Installation

```
ember install ember-cli-babel
```

## Compatibility

- ember-cli-babel 7.x requires ember-cli 2.13 or above

## Usage

This plugin should work without any configuration after installing. By default
it will take every `.js` file in your project and run it through the Babel
transpiler to convert your ES6 code to code supported by your target browsers
(as specified in `config/targets.js` in ember-cli >= 2.13). Running non-ES6
code through the transpiler shouldn't change the code at all (likely just a
format change if it does).

If you need to customize the way that `babel-preset-env` configures the plugins
that transform your code, you can do it by passing in any of the
[babel/babel-preset-env options](https://github.com/babel/babel-preset-env/tree/v1.6.1#options).
*Note: `.babelrc` files are ignored by default.*

Example (configuring babel directly):

```js
// ember-cli-build.js

let app = new EmberApp({
  babel: {
    // enable "loose" mode
    loose: true,
    // don't transpile generator functions
    exclude: [
      'transform-regenerator',
    ],
    plugins: [
      'transform-object-rest-spread'
    ]
  }
});
```

Example (configuring ember-cli-babel itself):

```js
// ember-cli-build.js

let app = new EmberApp({
  'ember-cli-babel': {
    compileModules: false
  }
});
```

### Options

There are a few different options that may be provided to ember-cli-babel.
These options are typically set in an apps `ember-cli-build.js` file, or in an
addon or engine's `index.js`.

```ts
type BabelPlugin = string | [string, any] | [any, any];

interface EmberCLIBabelConfig {
  /**
    Configuration options for babel-preset-env.
    See https://github.com/babel/babel-preset-env/tree/v1.6.1#options for details on these options.
  */
  babel?: {
    spec?: boolean;
    loose?: boolean;
    debug?: boolean;
    include?: string[];
    exclude?: string[];
    useBuiltIns?: boolean;
    sourceMaps?: boolean | "inline" | "both";
    plugins?: BabelPlugin[];
  };

  /**
    Configuration options for ember-cli-babel itself.
  */
  'ember-cli-babel'?: {
    includePolyfill?: boolean;
    includeExternalHelpers?: boolean;
    compileModules?: boolean;
    disableDebugTooling?: boolean;
    disablePresetEnv?: boolean;
    disableEmberModulesAPIPolyfill?: boolean;
    disableDecoratorTransforms?: boolean;
    extensions?: string[];
  };
}
```

The exact location you specify these options varies depending on the type of
project you're working on. As a concrete example, to add
`babel-plugin-transform-object-rest-spread` so that your project can use object
rest/spread syntax, you would do something like this in an app:

```js
// ember-cli-build.js
let app = new EmberApp(defaults, {
  babel: {
    plugins: ['transform-object-rest-spread']
  }
});
```

In an engine:
```js
// index.js
module.exports = EngineAddon.extend({
  babel: {
    plugins: ['transform-object-rest-spread']
  }
});
```

In an addon:
```js
// index.js
module.exports = {
  options: {
    babel: {
      plugins: ['transform-object-rest-spread']
    }
  }
};
```

#### Polyfill

Babel comes with a polyfill that includes a custom [regenerator
runtime](https://github.com/facebook/regenerator/blob/master/runtime.js) and
[core-js](https://github.com/zloirock/core-js). Many transformations will work
without it, but for full support you may need to include the polyfill in your
app.

To include it in your app, pass `includePolyfill: true` in your `ember-cli-babel` options.

```js
// ember-cli-build.js

let app = new EmberApp(defaults, {
  'ember-cli-babel': {
    includePolyfill: true
  }
});
```

#### External Helpers

Babel often includes helper functions to handle some of the more complex logic
in codemods. These functions are inlined by default, so they are duplicated in
every file that they are used in, which adds some extra weight to final builds.

Enabling `includeExternalHelpers` will cause Babel to import these helpers from
a shared module, reducing app size overall. This option is available _only_ to
the root application, because it is a global configuration value due to the fact
that there can only be one version of helpers included.

Note that there is currently no way to whitelist or blacklist helpers, so all
helpers will be included, even ones which are not used. If your app is small,
this could add to overall build size, so be sure to check.

`ember-cli-babel` will attempt to include helpers if it believes that it will
lower your build size, using a number of heuristics. You can override this to
force inclusion or exclusion of helpers in your app by passing `true` or `false`
to `includeExternalHelpers` in your `ember-cli-babel` options.

```js
// ember-cli-build.js

let app = new EmberApp(defaults, {
  'ember-cli-babel': {
    includeExternalHelpers: true
  }
});
```

#### Enabling Source Maps

Babel generated source maps will enable you to debug your original ES6 source
code. This is disabled by default because it will slow down compilation times.

To enable it, pass `sourceMaps: 'inline'` in your `babel` options.

```js
// ember-cli-build.js

let app = new EmberApp(defaults, {
  babel: {
    sourceMaps: 'inline'
  }
});
```

#### Modules

Older versions of Ember CLI (`< 2.12`) use its own ES6 module transpiler.
Because of that, this plugin disables Babel module compilation by blacklisting
that transform when running under affected ember-cli versions. If you find that
you want to use the Babel module transform instead of the Ember CLI one, you'll
have to explicitly set `compileModules` to `true` in your configuration. If
`compileModules` is anything other than `true`, this plugin will leave the
module syntax compilation up to Ember CLI.

#### Disabling Debug Tooling Support

If for some reason you need to disable this debug tooling, you can opt-out via
configuration.

In an app that would look like:

```js
// ember-cli-build.js
module.exports = function(defaults) {
  let app = new EmberApp(defaults, {
    'ember-cli-babel': {
      disableDebugTooling: true
    }
  });

  return app.toTree();
}
```

### Addon usage

#### Adding Custom Plugins

You can add custom plugins to be used during transpilation of the `addon/` or
`addon-test-support/` trees by ensuring that your addon's `options.babel` is
properly populated (as mentioned above in the `Options` section).

#### Additional Trees

For addons which want additional customizations, they are able to interact with
this addon directly.

```ts
interface EmberCLIBabel {
  /**
    Used to generate the options that will ultimately be passed to babel itself.
  */
  buildBabelOptions(config?: EmberCLIBabelConfig): Opaque;

  /**
    Supports easier transpilation of non-standard input paths (e.g. to transpile
    a non-addon NPM dependency) while still leveraging the logic within
    ember-cli-babel for transpiling (e.g. targets, preset-env config, etc).
  */
  transpileTree(inputTree: BroccoliTree, config?: EmberCLIBabelConfig): BroccoliTree;

  /**
    Used to determine if a given plugin is required by the current target configuration.
    Does not take `includes` / `excludes` into account.

    See https://github.com/babel/babel-preset-env/blob/master/data/plugins.json for the list
    of known plugins.
  */
  isPluginRequired(pluginName: string): boolean;
}
```

#### `buildBabelOptions` usage

```js
// find your babel addon (can use `this.findAddonByName('ember-cli-babel')` in ember-cli@2.14 and newer)
let babelAddon = this.addons.find(addon => addon.name === 'ember-cli-babel');

// create the babel options to use elsewhere based on the config above
let options = babelAddon.buildBabelOptions(config)

// now you can pass these options off to babel or broccoli-babel-transpiler
require('babel-core').transform('something', options);
```

#### `transpileTree` usage

```js
// find your babel addon (can use `this.findAddonByName('ember-cli-babel')` in ember-cli@2.14 and newer)
let babelAddon = this.addons.find(addon => addon.name === 'ember-cli-babel');

// invoke .transpileTree passing in the custom input tree
let transpiledCustomTree = babelAddon.transpileTree(someCustomTree);
```

### Debug Tooling

In order to allow apps and addons to easily provide good development mode
ergonomics (assertions, deprecations, etc) but still perform well in production
mode ember-cli-babel automatically manages stripping / removing certain debug
statements. This concept was originally proposed in [ember-cli/rfcs#50](https://github.com/ember-cli/rfcs/pull/50),
but has been slightly modified during implementation (after researching what works well and what does not).

#### Debug Macros

To add convienient deprecations and assertions, consumers (in either an app or an addon) can do the following:

```js
import { deprecate, assert } from '@ember/debug';

export default Ember.Component.extend({
  init() {
    this._super(...arguments);
    deprecate(
      'Passing a string value or the `sauce` parameter is deprecated, please pass an instance of Sauce instead',
      false,
      { until: '1.0.0', id: 'some-addon-sauce' }
    );
    assert('You must provide sauce for x-awesome.', this.sauce);
  }
})
```

In testing and development environments those statements will be executed (and
assert or deprecate as appropriate), but in production builds they will be
inert (and stripped during minification).

The following are named exports that are available from `@ember/debug`:

* `function deprecate(message: string, predicate: boolean, options: any): void` - Results in calling `Ember.deprecate`.
* `function assert(message: string, predicate: boolean): void` - Results in calling `Ember.assert`.
* `function warn(message: string, predicate: boolean): void` - Results in calling `Ember.warn`.

#### General Purpose Env Flags

In some cases you may have the need to do things in debug builds that isn't
related to asserts/deprecations/etc. For example, you may expose certain API's
for debugging only. You can do that via the `DEBUG` environment flag:

```js
import { DEBUG } from '@glimmer/env';

const Component = Ember.Component.extend();

if (DEBUG) {
  Component.reopen({
    specialMethodForDebugging() {
      // do things ;)
    }
  });
}
```

In testing and development environments `DEBUG` will be replaced by the boolean
literal `true`, and in production builds it will be replaced by `false`. When
ran through a minifier (with dead code elimination) the entire section will be
stripped.

Please note, that these general purpose environment related flags (e.g. `DEBUG`
as a boolean flag) are imported from `@glimmer/env` not from an `@ember`
namespace.

#### Parallel Builds

By default, [broccoli-babel-transpiler] will attempt to spin up several
sub-processes (~1 per available core), to achieve parallelization. (Once Node.js
has built-in worker support, we plan to utilize it.) This yields significant Babel
build time improvements.

Unfortunately, some Babel plugins may break this functionality.
When this occurs, we gracefully fallback to the old serial strategy.

To have the build fail when failing to do parallel builds, opt-in is via:

```js
let app = new EmberAddon(defaults, {
  'ember-cli-babel': {
    throwUnlessParallelizable: true
  }
});
```
*Note: Future versions will enable this flag by default.*

Read more about [broccoli parallel transpilation].

[broccoli-babel-transpiler]: https://github.com/babel/broccoli-babel-transpiler
[broccoli parallel transpilation]: https://github.com/babel/broccoli-babel-transpiler#parallel-transpilation
