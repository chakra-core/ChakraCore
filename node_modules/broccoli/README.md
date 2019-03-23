# Broccoli

<img src="logo/broccoli-logo.generated.png" align="right" height="150">

[![Build Status](https://travis-ci.org/broccolijs/broccoli.svg?branch=master)](https://travis-ci.org/broccolijs/broccoli)
[![Build status](https://ci.appveyor.com/api/projects/status/jd3ts93gryjeqclf/branch/master?svg=true)](https://ci.appveyor.com/project/joliss/broccoli/branch/master)

A fast, reliable asset pipeline, supporting constant-time rebuilds and compact
build definitions. Comparable to the Rails asset pipeline in scope, though it
runs on Node and is backend-agnostic. For background and architecture, see the
[introductory blog post](http://www.solitr.com/blog/2014/02/broccoli-first-release/).

For the command line interface, see
[broccoli-cli](https://github.com/broccolijs/broccoli-cli).

## Installation

```bash
npm install --save-dev broccoli
npm install --global broccoli-cli
```

## Brocfile.js

A `Brocfile.js` file in the project root contains the build specification. It
should export a function that returns a tree. Note: the Brocfile historically
could export a tree/string directly, however this is now deprecated in favor
of a function that can receive [options](#options)

A tree can be any string representing a directory path, like `'app'` or
`'src'`. Or a tree can be an object conforming to the [Plugin API
Specification](#plugin-api-specification). A `Brocfile.js` will usually
directly work with only directory paths, and then use the plugins in the
[Plugins](#plugins) section to generate transformed trees.

The following simple `Brocfile.js` would export the `app/` subdirectory as a
tree:

```js
export default () => 'app';
```

With that Brocfile, the build result would equal the contents of the `app`
tree in your project folder. For example, say your project contains these
files:

    app
    ├─ main.js
    └─ helper.js
    Brocfile.js
    package.json
    …

Running `broccoli build the-output` (a command provided by
[broccoli-cli](https://github.com/broccolijs/broccoli-cli)) would generate
the following folder within your project folder:

    the-output
    ├─ main.js
    └─ helper.js

### Options

The function that is exported from `module.exports` is passed an options hash by Broccoli
that can be used when assembling the build.

The options hash is populated by the CLI environment when running `broccoli build`
or `broccoli serve`. It currently only accepts a single option `--environment`, which
is passed as `env` in the `options` hash.

Additionally `--prod` and `--dev` are available aliases to `--environment=production`
and `--environment=development` respectively.

* `options`:
    * `env`: Defaults to `development`, and can be overridden with the CLI argument `--environment=X`

For example:
```js
export default (options) => {
    // tree = ... assemble tree

    // In production environment, minify the files
    if (options.env === 'production') {
        tree = minify(tree);
    }

    return tree;
}
```

### TypeScript Support

A `Brocfile.ts` can be used in place of a `Brocfile.js` and Broccoli will automatically parse this
through [ts-node](https://www.npmjs.com/package/ts-node) to provide [TypeScript](https://www.typescriptlang.org)
support. This allows developers to leverage type information when assembling a build pipeline. By default,
Broccoli provides [type information](lib/index.d.ts) for the [options](#options) object passed to the build function.

```ts
import { BrocfileOptions } from 'broccoli';

export default (options: BrocfileOptions) => {
  // tree = ... assemble tree

  // In production environment, minify the files
  if (options.env === 'production') {
    tree = minify(tree);
  }

  return tree;
};

```

Typescript by default only allows the [ES6 modules](https://nodejs.org/api/esm.html) `import/export` syntax to work
when importing ES6 modules. In order to import a CommonJS module (one that uses `require() or module.exports`, you must
use the following syntax:

```ts
import foo = require('foo');

export = 'bar';
```

You'll note the syntax is slightly different from the ESM syntax, but reads fairly well.

### Using plugins in a `Brocfile.js`

The following `Brocfile.js` exports the `app/` subdirectory as `appkit/`:

```js
// Brocfile.js
import Funnel from 'broccoli-funnel';

export default () => new Funnel('app', {
  destDir: 'appkit'
})
```

Broccoli supports [ES6 modules](https://nodejs.org/api/esm.html) via [esm](https://www.npmjs.com/package/esm) for
`Brocfile.js`. Note, TypeScript requires the use of a different syntax, see the TypeScript section above.

You can also use regular CommonJS `require` and `module.exports` if you prefer, however ESM is the future of Node,
and the recommended syntax to use.

That example uses the plugin [`broccoli-funnel`](https://www.npmjs.com/package/broccoli-funnel).
In order for the `import` call to work, you must first put the plugin in your `devDependencies` and install it, with

    npm install --save-dev broccoli-funnel

With the above `Brocfile.js` and the file tree from the previous example,
running `broccoli build the-output` would generate the following folder:

    the-output
    └─ appkit
       ├─ main.js
       └─ helper.js

## Plugins

You can find plugins under the [broccoli-plugin keyword](https://www.npmjs.org/browse/keyword/broccoli-plugin) on npm.

## Using Broccoli Programmatically

In addition to using Broccoli via the combination of `broccoli-cli` and a `Brocfile.js`, you can also use Broccoli programmatically to construct your own build output via the `Builder` class. The `Builder` is one of the core APIs in Broccoli, and is responsible for taking a graph of Broccoli nodes and producing an actual build artifact (i.e. the output usually found in your `dist` directory after you run `broccoli build`). The output of a `Builder`'s `build` method is a Promise that resolves when all the operations in the graph are complete. You can use this promise to chain together additional operations (such as error handling or cleanup) that will execute once the build step is complete.

By way of example, let's assume we have a graph of Broccoli nodes constructed via a combination of `Funnel` and `MergeTrees`:

```js
// non Brocfile.js, regular commonjs
const Funnel = require('broccoli-funnel');
const Merge = require('broccoli-merge');

const html = new Funnel(appRoot, {
  files: ['index.html'],
  annotation: 'Index file'
})

const js = new Funnel(appRoot, {
  files: ['app.js'],
  destDir: '/assets',
  annotation: 'JS Files'
});

const css = new Funnel(appRoot, {
  srcDir: 'styles',
  files: ['app.css'],
  destDir: '/assets',
  annotation: 'CSS Files'
});

const public = new Funnel(appRoot, {
  annotation: 'Public Files'
});

const tree = new Merge([html, js, css, public]);
```

At this point, `tree` is a graph of nodes, each of which can represent either an input or a transformation that we want to perform. In other words, `tree` is an abstract set of operations, *not* a concrete set of output files.

In order to perform all the operations described in `tree`, we need to do the following:
- construct a `Builder` instance, passing in the graph we constructed before
- call the `build` method, which will traverse the graph, performing each operation and eventually writing the output to a temporary folder indicated by `builder.outputPath`

Since we typically want do more than write to a temporary folder, we'll also use a library called `TreeSync` to sync the contents of the temp file with our desired output directory. Finally, we'll clean up the temporary folder once all our operations are complete:

```js
const { Builder } = require('broccoli');
const TreeSync = require('tree-sync');
const Merge = require('broccoli-merge');
// ...snip...
const tree = new Merge([html, js, css, public]);

const builder = new Builder(tree);

const outputDir = 'dist';
const outputTree = new TreeSync(builder.outputPath, outputDir);

builder.build()
  .then(() => {
    // Calling `sync` will synchronize the contents of the builder's `outPath` with our output directory.
    return outputTree.sync();
  })
  .then(() => {
    // Now that we're done with the build, clean up any temporary files were created
    return builder.cleanup();
  })
  .catch(err => {
    // In case something in this process fails, we still want to ensure that we clean up the temp files
    console.log(err);
    return builder.cleanup();
  });
```

### Running Broccoli, Directly or Through Other Tools

* [broccoli-timepiece](https://github.com/rjackson/broccoli-timepiece)
* [grunt-broccoli](https://github.com/embersherpa/grunt-broccoli)
* [grunt-broccoli-build](https://github.com/ericf/grunt-broccoli-build)

### Helpers

Shared code for writing plugins.

* [broccoli-plugin](https://github.com/broccolijs/broccoli-plugin)
* [broccoli-caching-writer](https://github.com/rjackson/broccoli-caching-writer)
* [broccoli-filter](https://github.com/broccolijs/broccoli-filter)

## Plugin API Specification

See [docs/node-api.md](docs/node-api.md).

Also see [docs/broccoli-1-0-plugin-api.md](docs/broccoli-1-0-plugin-api.md) on
how to upgrade from Broccoli 0.x to the Broccoli 1.x API.

## Security

* Do not run `broccoli serve` on a production server. While this is
  theoretically safe, it exposes a needlessly large amount of attack surface
  just for serving static assets. Instead, use `broccoli build` to precompile
  your assets, and serve the static files from a web server of your choice.

## Get Help

* IRC: `#broccolijs` on Freenode. Ask your question and stick around for a few
  hours. Someone will see your message eventually.
* Twitter: mention @jo_liss with your question
* GitHub: Open an issue on a specific plugin repository, or on this
  repository for general questions.

## License

Broccoli was originally written by [Jo Liss](http://www.solitr.com/) and is
licensed under the [MIT license](LICENSE).

The Broccoli logo was created by [Samantha Penner
(Miric)](http://mirics.deviantart.com/) and is licensed under [CC0
1.0](https://creativecommons.org/publicdomain/zero/1.0/).
