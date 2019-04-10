# master

# 2.0.1

* Fix various issues resulting in out of memory errors during instrumentation node traversal.

# 2.0.0

* Update `sane` to ensure no native dependencies are needed.
* Cleanup `Builder.prototype.build` to properly return `Promise<void>` (removing the `outputNodeWrapper`).
* Add documentation for programmatic `Builder` usage.
* Update internal dependencies to latest versions.
* Remove usages of `RSVP` (in favor of using native promises).
* Add support for Node 10.
* Drop support for Node versions older than 6.
  * Drop Node 4 support.
  * Drop Node 0.12 support.
* Add visualization support via heimdalljs.
* Ensure that mid-build cancelation avoids extra work.
* Add `--overwrite` option to the command line interface which clobbers any
  existing directory contents with the contents of the new build.
* Add `--cwd` option to the command line interface which allows customizing the
  builders working directory (and where the `Brocfile.js` is looked up from).
* Add `--output-path` option to the command line interface.
* Add `--watch` option to `build` sub-command.
* Add `--no-watch` option to `serve` sub-command.
* Add `--watcher` option to allow configuration of the watcher to be used. Currently supported values are polling, watchman, node, events.
* General code cleanup and modernization.

# 1.1.4

* Roll back broccoli-slow-trees dependency

# 1.1.3

* Update dependencies

# 1.1.2

* Update findup-sync dependency

# 1.1.1

* Fix option parsing for `--port`

# 1.1.0

* Add `needsCache` to `pluginInterface`. Allows opting out of cache directory
  creation.

# 1.0.0

* Release without change

# 1.0.0-beta.8

* Builder throws an error when a watched input directory is missing
* Rework watcher
* Pull broccoli-sane-watcher functionality into core
* Update findup-sync dependency

# 1.0.0-beta.7

* Remove wrong `postinstall` hook. This removes a spurious dependency on
  `multidep`.

# 1.0.0-beta.6

* Add `build` event to watcher

# 1.0.0-beta.5

* Remove redundant `beginBuild`/`endBuild` (formerly `start`/`end`) events on builder

# 1.0.0-beta.4

* Improve test suite

# 1.0.0-beta.3

* Minor cosmetic changes

# 1.0.0-beta.2

* Add `watcher.watch()` method. `Watcher` no longer automatically starts
  watching; instead, you must call this method explicitly. It returns a promise
  that is fulfilled if you later call `watcher.quit()`, or rejected if watching
  one of the source directories fails.

    - `server` will call `watcher.watch()` for you.
    - In contrast, `getMiddleware` expects a watcher that is already watching.

# 1.0.0-beta.1

* Drop support for plugins that implement only the old `.read/.rebuild` API
* Fail build when a source node is a file rather than a directory
* Fail build when a source node doesn't exist
* Builder API changes:

    - `new Builder` has a `tmpdir` option, which defaults to `os.tmpdir()`
      (typically `/tmp`); pass `{ tmpdir: './tmp' }` to get the old behavior
    - `.build()` no longer returns a promise to the output path; instead, the
      output path stored at `builder.outputPath` and doesn't change between
      builds
    - `start`, `end`, `nodeStart`, `nodeEnd` events renamed to
      `beginBuild`, `endBuild`, `beginNode`, `endNode`
    - Nodes passed to `nodeBegin`/`nodeEnd` arguments are "node wrapper"
      objects (also accessible at `builder.nodeWrappers`); timings now
      reside at `nodeWrapper.buildState.selfTime/totalTime` and are in
      milliseconds, not nanoseconds
    - `build()` no longer takes a `willReadStringTree` callback argument;
      instead, source directories are recorded at `builder.watchedPaths`

* Watcher API changes:

    - Add `watcher.quit()` method, which returns a promise until a running
      build has finished (if any)
    - Rename `watcher.current` to `watcher.currentBuild`, and remove
      `watcher.then`
    - Use `RSVP.EventTarget` instead of `EventEmitter` for events

* Build error objects have been changed to `Builder.BuildError` objects, which
  contain additional information at `err.broccoliPayload`

# 0.16.8

* Add builder hooks

# 0.16.7

* Export watcher and middleware as `Watcher` and `getMiddleware`

# 0.16.6

* Export watcher and middleware

# 0.16.5

* On BROCCOLI_WARN_READ_API=y, print deprecation warning for .rebuild as well

# 0.16.4

* Return server objects for easier extensibility

# 0.16.3

* Do not silently swallow errors in change/error event handlers

# 0.16.2

* Add missing dependency

# 0.16.1

* Add Node interface to Builder, to enable building visualizations
* Export `Builder.getDescription(tree)` helper function
* Add footer to directory listings, so people know where they come from

# 0.16.0

* Remove built-in LiveReload server; tools like Ember CLI inject LiveReload scripts, which is generally preferable because it doesn't need a separate port

# 0.15.4

* Send `Cache-Control` header for directory listings and redirects
* Honor `liveReloadPath` middleware option in directory listings as well
* Add `autoIndex` middleware option to disable directory listings

# 0.15.3

* Correctly display multi-line error messages

# 0.15.2

* Add ability to inject live-reload script into error messages

# 0.15.1

* Hide API warnings behind $BROCCOLI_WARN_READ_API env flag
* Add support for new error API
* Fail fast if `build` output directory already exists

# 0.15.0

* Print deprecation warnings for plugins only providing old `.read` API

# 0.14.0

* Add support for new [`.rebuild`
  API](https://github.com/broccolijs/broccoli/blob/master/docs/new-rebuild-api.md),
  in addition to existing (now deprecated) `.read` API

# 0.13.6

* Throw helpful error when we encounter as-yet unsupported [`.rebuild`-based
  plugins](https://github.com/broccolijs/broccoli/blob/master/docs/new-rebuild-api.md)

# 0.13.5

* Add missing `var`

# 0.13.4

* More detailed error message when a tree object is invalid
* Watcher no longer rebuilds forever when a very early build error occurs

# 0.13.3

* Fix SIGINT/SIGTERM (Ctrl+C) handling to avoid leaking tmp files

# 0.13.2

* Extract slow trees printout into broccoli-slow-trees package
* Allow the tree `cleanup` method to be asynchronous (by returning a promise).

# 0.13.1

* Update dependencies to fix
  [various low-severity vulnerabilities](https://github.com/broccolijs/broccoli/issues/196)
  in `broccoli serve`

# 0.13.0

* Dereference symlinks in `broccoli build` output by copying the files or
  directories they point to into place
* Sort entries when browsing directories in middleware

# 0.12.3

* Exclude `logo` and `test` directories from npm distribution

# 0.12.2

* Fix directory handling in server on Windows

# 0.12.1

* Show directory listing with `broccoli serve` when there is no `index.html`

# 0.12.0

* Add `willReadStringTree` callback argument to `Builder::build` and retire
  `Builder::treesRead`
* Update `Watcher` and `Builder` interaction to prevent double builds.
* Avoid unhandled rejected promise
* Fix trailing slash handling in server on Windows

# 0.11.0

* Change `Watcher`'s `change` event to provide the full build results (instead of just the directory).
* Add slow tree logging to `broccoli serve` output.
* Add logo

# 0.10.0

* Move process.exit listener out of builder into server
* Change `Builder::build()` method to return a `{ directory, graph }` hash
  instead of only the directory, where `graph` contains the output directories
  and timings for each tree
* Avoid keeping file streams open in server, to fix EBUSY issues on Windows

# 0.9.0

* `Brocfile.js` now exports a tree, not a function ([sample diff](https://gist.github.com/joliss/15630762fa0f43976418))

# 0.8.0

* Extract bowerTrees into broccoli-bower plugin ([sample diff](https://github.com/joliss/broccoli-sample-app/commit/829e869f795012c08f5643a047b3f46c61dd0168))

# 0.7.2

* Update dependencies

# 0.7.1

* Do not use hardlinks in bower implementation

# 0.7.0

* Remove `broccoli.MergedTree`; it has been extracted into broccoli-merge-trees
  ([sample diff](https://github.com/joliss/broccoli-sample-app/commit/b6b30d5dd23ddf86d8b95b1440b2937de1b8bbcd#diff-ec6fb87583b2323d013c3e30c0a5084dL50))

# 0.6.0

* Disallow returning arrays from Brocfile.js, in favor of broccoli-merge-trees
  plugin ([sample diff](https://github.com/joliss/broccoli-sample-app/commit/b6b30d5dd23ddf86d8b95b1440b2937de1b8bbcd))

# 0.5.0

* Remove `broccoli.makeTree('foo')` in favor of string literals (just `'foo'`)
  ([sample diff](https://github.com/joliss/broccoli-sample-app/commit/ccd03da8e803a15fdd50e47c0ee71f9bbcfd911e))
* Remove `broccoli.Reader`
* Add `--version` command line option

# 0.4.3

* Correct mis-publish on npm

# 0.4.2

* Preserve value/error on Watcher::current promise
* This version has been unpublished due to a mis-publish

# 0.4.1

* Extract `broccoli.helpers` into broccoli-kitchen-sink-helpers package

# 0.3.1

* Report unhandled errors in the watcher
* Add support for `.treeDir` property on error objects
* Improve watcher logic to stop double builds when build errors happen

# 0.3.0

* Bind to `localhost` instead of `0.0.0.0` (whole wide world) by default

# 0.2.6

* Overwrite mis-pushed release

# 0.2.5

* Refactor watcher logic to use promises
* Turn the hapi server into a connect middleware

# 0.2.4

* Use smaller `bower-config` package instead of `bower` to parse `bower.json`
  files

# 0.2.3

* Add `--port`, `--host`, and `--live-reload-port` options to `serve` command

# 0.2.2

* Update hapi dependency to avoid file handle leaks, causing EMFILE errors

# 0.2.1

* In addition to `Brocfile.js`, accept lowercase `brocfile.js`
* Fix error reporting for string exceptions

# 0.2.0

* Rename `Broccolifile.js` to `Brocfile.js`
* Change default port from 8000 to 4200

# 0.1.1

* Make `tree.cleanup` non-optional
* Rename `broccoli.read` to `broccoli.makeTree`

# 0.1.0

* Bump to indicate beta status
* Remove unused `helpers.walkSync` (now in node-walk-sync)

# 0.0.13

* Extract `Transformer` into `broccoli-transform` package (now "`Transform`")
* Extract `Filter` into `broccoli-filter` package

# 0.0.12

* In plugin (tree) API, replace `.afterBuild` with `.cleanup`
* Move temporary directories out of the way

# 0.0.11

* Extract `factory.env` into broccoli-env package
* Eliminate `factory` argument to Broccolifile

# 0.0.10

* Change to a `.read`-based everything-is-a-tree architecture
* Various performance improvements
* Various plugin API changes
* Add `MergedTree`
* Broccolifile may now return an array of trees, which will be merged
* Expose `broccoli.bowerTrees()`, which will hopefully be redesigned and go
  away again
* Remove `Component` base class
* Remove `CompilerCollection` and `Compiler` base class; use a `Transformer`
* Remove `Tree::addTransform`, `Tree::addTrees`, and `Tree::addBower`
* `Builder::build` now has a promise interface as well

# 0.0.9

* Expect a `Tree`, not a `Builder`, returned from Broccolifile.js

# 0.0.8

* Fold `Reader` into `Tree`
* Replace `PreprocessorPipeline` and `Preprocessor` with `Filter`; each
  `Filter` is added directly on the tree or builder with `addTransform`

# 0.0.7

* Bind to `0.0.0.0` instead of `localhost`
* Add `factory.env` based on `$BROCCOLI_ENV`
* Do not fail on invalid Cookie header
* Use promises instead of callbacks in all external APIs

# 0.0.6

* Here be dragons
