# broccoli-persistent-filter

[![Greenkeeper badge](https://badges.greenkeeper.io/stefanpenner/broccoli-persistent-filter.svg)](https://greenkeeper.io/)
[![Build Status](https://travis-ci.org/stefanpenner/broccoli-persistent-filter.svg?branch=master)](https://travis-ci.org/stefanpenner/broccoli-persistent-filter)
[![Build status](https://ci.appveyor.com/api/projects/status/gvt0rheb1c2c4jwd/branch/master?svg=true)](https://ci.appveyor.com/project/embercli/broccoli-persistent-filter/branch/master)

Helper base class for Broccoli plugins that map input files into output files. Except with a persistent cache to fast restarts.
one-to-one.

## API

```js
class Filter {
  /**
   * Abstract base-class for filtering purposes.
   *
   * Enforces that it is invoked on an instance of a class which prototypically
   * inherits from Filter, and which is not itself Filter.
   */
  constructor(inputNode: BroccoliNode, options: FilterOptions): Filter;

  /**
   * Abstract method `processString`: must be implemented on subclasses of
   * Filter.
   *
   * The resolved return value can either be an object or a string.
   *
   * An object can be used to cache additional meta-data that is not part of the
   * final output. When an object is returned, the `.output` property of that
   * object is used as the resulting file contents.
   *
   * When a string is returned it is used as the file contents.
   */
  abstract processString(contents: string, relativePath: string): {string | object };

  /**
   * Virtual method `getDestFilePath`: determine whether the source file should
   * be processed, and optionally rename the output file when processing occurs.
   *
   * Return `null` to pass the file through without processing. Return
   * `relativePath` to process the file with `processString`. Return a
   * different path to process the file with `processString` and rename it.
   *
   * By default, if the options passed into the `Filter` constructor contain a
   * property `extensions`, and `targetExtension` is supplied, the first matching
   * extension in the list is replaced with the `targetExtension` option's value.
   */
  virtual getDestFilePath(relativePath: string): string;

  /**
   * Method `postProcess`: may be implemented on subclasses of
   * Filter.
   *
   * This method can be used in subclasses to do processing on the results of
   * each files `processString` method.
   *
   * A common scenario for this is linting plugins, where on initial build users
   * expect to get console warnings for lint errors, but we do not want to re-lint
   * each file on every boot (since most of them will be able to be served from the
   * cache).
   *
   * The `.output` property of the return value is used as the emitted file contents.
   */
  postProcess(results: object, relativePath: string): object

}
```

### Options

* `extensions`: An array of file extensions to process, e.g. `['md', 'markdown']`.
* `targetExtension`: The file extension of the corresponding output files, e.g.
  `'html'`.
* `inputEncoding`: The character encoding used for reading input files to be
  processed (default: `'utf8'`). For binary files, pass `null` to receive a
  `Buffer` object in `processString`.
* `outputEncoding`: The character encoding used for writing output files after
  processing (default: `'utf8'`). For binary files, pass `null` and return a
  `Buffer` object from `processString`.
* `async`: Whether the `create` and `change` file operations are allowed to
  complete asynchronously (true|false, default: false)
* `name`, `annotation`: Same as
  [broccoli-plugin](https://github.com/broccolijs/broccoli-plugin#new-plugininputnodes-options);
  see there.
* `concurrency`: Used with `async: true`. The number of operations that can be
  run concurrently. This overrides the value set with `JOBS=n` environment variable.
  (default: the number of detected CPU cores - 1, with a min of 1)

All options except `name` and `annotation` can also be set on the prototype
instead of being passed into the constructor.

### Example Usage

```js
var Filter = require('broccoli-persistent-filter');

Awk.prototype = Object.create(Filter.prototype);
Awk.prototype.constructor = Awk;
function Awk(inputNode, search, replace, options) {
  options = options || {};
  Filter.call(this, inputNode, {
    annotation: options.annotation
  });
  this.search = search;
  this.replace = replace;
}

Awk.prototype.extensions = ['txt'];
Awk.prototype.targetExtension = 'txt';

Awk.prototype.processString = function(content, relativePath) {
  return content.replace(this.search, this.replace);
};
```

In `Brocfile.js`, use your new `Awk` plugin like so:

```
var node = new Awk('docs', 'ES6', 'ECMAScript 2015');

module.exports = node;
```

## Persistent Cache

Adding persist flag allows a subclass to persist state across restarts. This exists to mitigate the upfront cost of some more expensive transforms on warm boot. __It does not aim to improve incremental build performance, if it does, it should indicate something is wrong with the filter or input filter in question.__

By default, if the the `CI=true` environment variable is set, peristent caches are disabled.
To force persistent caches on CI, please set the `FORCE_PERSISTENCE_IN_CI=true` environment variable;

### How does it work?

It does so but establishing a 2 layer file cache. The first layer, is the entire bucket.
The second, `cacheKeyProcessString` is a per file cache key.

Together, these two layers should provide the right balance of speed and sensibility.

The bucket level cacheKey must be stable but also never become stale. If the key is not
stable, state between restarts will be lost and performance will suffer. On the flip-side,
if the cacheKey becomes stale changes may not be correctly reflected.

It is configured by subclassing and refining `cacheKey` method. A good key here, is
likely the name of the plugin, its version and the actual versions of its dependencies.

```js
Subclass.prototype.cacheKey = function() {
 return md5(Filter.prototype.call(this) + inputOptionsChecksum + dependencyVersionChecksum);
}
```

The second key, represents the contents of the file. Typically the base-class's functionality
is sufficient, as it merely generates a checksum of the file contents. If for some reason this
is not sufficient, it can be re-configured via subclassing.

```js
Subclass.prototype.cacheKeyProcessString = function(string, relativePath) {
  return superAwesomeDigest(string);
}
```

It is recommended that persistent re-builds is opt-in by the consuming plugin author, as if no reasonable cache key can be created it should not be used.

```js
var myTree = new SomePlugin('lib', { persist: true });
```

### Warning

By using the persistent cache, a lot of small files will be created on the disk without being deleted.
This might use all the inodes of your disk.
You need to make sure to clean regularly the old files or configure your system to do so.

On OSX, [files that aren't accessed in three days are deleted from `/tmp`](http://superuser.com/a/187105).
On systems using systemd, [systemd-tmpfiles](https://www.freedesktop.org/software/systemd/man/systemd-tmpfiles.html) should already be present and regularly clean up the `/tmp` directory.
On Debian-like systems, you can use [tmpreaper](https://packages.debian.org/stable/tmpreaper).
On RedHat-like systems, you can use [tmpwatch](https://fedorahosted.org/tmpwatch/).

By default, the files are stored in the [operating system's default directory for temporary files](https://nodejs.org/api/os.html#os_os_tmpdir),
but you can change this location by setting the `BROCCOLI_PERSISTENT_FILTER_CACHE_ROOT` environment variable to the path of another folder.

To clear the persistent cache on any particular build, set the `CLEAR_BROCCOLI_PERSISTENT_FILTER_CACHE` environment variable to `true` like so:

```sh
CLEAR_BROCCOLI_PERSISTENT_FILTER_CACHE=true ember serve
```

## FAQ

### Upgrading from 0.1.x to 1.x

You must now call the base class constructor. For example:

```js
// broccoli-filter 0.1.x:
function MyPlugin(inputTree) {
  this.inputTree = inputTree;
}

// broccoli-filter 1.x:
function MyPlugin(inputNode) {
  Filter.call(this, inputNode);
}
```

Note that "node" is simply new terminology for "tree".

### Source Maps

**Can this help with compilers that are almost 1:1, like a minifier that takes
a `.js` and `.js.map` file and outputs a `.js` and `.js.map` file?**

Not at the moment. I don't know yet how to implement this and still have the
API look beautiful. We also have to make sure that caching works correctly, as
we have to invalidate if either the `.js` or the `.js.map` file changes. My
plan is to write a source-map-aware uglifier plugin to understand this use
case better, and then extract common code back into this `Filter` base class.
