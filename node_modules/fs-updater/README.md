# node-fs-updater

[![Build Status](https://travis-ci.org/broccolijs/node-fs-updater.svg?branch=master)](https://travis-ci.org/broccolijs/node-fs-updater)
[![Build status](https://ci.appveyor.com/api/projects/status/5sb039bjee4q3obw?svg=true)](https://ci.appveyor.com/project/joliss/node-fs-updater)


Repeatedly write an in-memory directory tree to disk, with incremental updating.

## Installation

This package requires Node version 6.0.0 or newer.

```bash
npm install --save fs-updater
```

## Usage

```js
let FSUpdater = require("fs-updater");
let { File, Directory, DirectoryIndex } = FSUpdater;

// output_dir must either be an empty directory or not exist at all
let fsUpdater = new FSUpdater('./output_dir')
```

Let's create the following directory structure, where `->` indicates a symlink
(or a copy on Windows):

```
output_dir
├── file -> /path/to/some/file
├── dir1/ -> /path/to/some/dir
└── dir2/
    └── another_file -> /path/to/another/file
```

```js
let dir = new DirectoryIndex([
  ['file', new File('/path/to/some/file')],
  ['dir1', new Directory('/path/to/some/dir')],
  ['dir2', new DirectoryIndex([
    ['another_file', new File('/path/to/another/file')]
  ])]
]);

// Write it to ./output_dir
fsUpdater.update(dir);
```

Now let's create an updated similar directory structure:

```
.
├── file -> /path/to/some/file
└── dir1/ -> /path/to/some/dir
```

```js
dir = new DirectoryIndex([
  ['file', new File('/path/to/some/file')],
  ['dir1', new Directory('/path/to/some/dir')]
]);

// Now update output_dir incrementally
fsUpdater.update(dir);
```

### Object re-use

It is recommended that you rebuild all your `File`, `Directory` and
`DirectoryIndex` objects from scratch each time you call `fsUpdater.update`. If
you re-use objects, the following rules apply:

First, do not mutate the objects that you pass into `FSUpdater`, or their
sub-objects. That is, after calling `fsUpdater.update(dir)`, you must no longer
call `dir.set(...)`.

Second, you may re-use unchanged `File`, `Directory` and `DirectoryIndex`
objects *only* if you know that the file contents they point to *recursively*
have not changed. This is typically only the case if they point into directories
that you control, and if those directories in turn contain no symlinks to
outside directories under the user's control.

For example, this is always OK:

```js
let file = new String('/the/file');
fsUpdater.update(new DirectoryIndex([
  ['file', file]
]);

// Create new File object with identical path
file = new String('/the/file');
fsUpdater.update(new DirectoryIndex([
  ['file', file]
]);
```

But this is only OK if the contents of `/the/file` have not changed between
calls to `.update`:

```js
let file = new String('/the/file');
fsUpdater.update(new DirectoryIndex([
  ['file', file]
]);

// Re-use the File object
fsUpdater.update(new DirectoryIndex([
  ['file', file]
]);
```

## Reference

* `FSUpdater`: An object used to repeatedly update an output directory.

  * `new FSUpdater(outputPath, options)`: Create a new `FSUpdater` object. The
    `outputPath` must be an empty directory or absent.

    It is important that the `FSUpdater` has exclusive access to the
    `outputPath` directory. `FSUpdater.prototype.update` calls
    [rimraf](https://github.com/isaacs/rimraf), which can be dangerous in the
    presence of symlinks if unexpected changes have been made to the
    `outputPath` directory.

    `options.canSymlink` (boolean): If true, use symlinks; if false, copy
    files and use junctions. If `null` (default), auto-detect.

  * `FSUpdater.prototype.update(directory)`: Update the `outputPath` directory
    to mirror the contents of the `directory` object, which is either a
    `DirectoryIndex` (an in-memory directory) or a `Directory` (a directory on
    disk).

    **Important note:** You may re-use `File` objects contained in the
    `DirectoryIndex` between repeated calls to `.update()` only if the file
    contents have not changed. Similarly, you may re-use `DirectoryIndex` and
    `Directory` objects only if no changes have been made to the directory or
    any files or subdirectories recursively, including those reachable through
    symlinks.

* `FSUpdater.DirectoryIndex`: A subclass of
  [Map](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Map)
  representing an in-memory directory; see the documentation there.

  `DirectoryIndex` objects map file names (`string` primitives, without paths)
  to `DirectoryIndex`, `Directory` or `File` objects.

* `FSUpdater.Directory`: A directory on disk. Think of this as an in-memory
  symlink to a directory.

  * `new Directory(path)`: Create a new `Directory` object pointing to
    the directory at `path`.

  * `Directory.prototype.valueOf()`: Return the `path`.

  * `Directory.prototype.getIndexSync()`: Read the physical directory and return a
    `DirectoryIndex`. The `DirectoryIndex` object is cached between repeated
    calls to `getIndexSync()`.

* `FSUpdater.File`: Represents a file on disk. Think of this as an in-memory
  symlink.

  * `new File(path)`: Create a new `File` object pointing to the file at `path`.

  * `File.prototype.valueOf()`: Return the `path`.

* `FSUpdater.makeFSObject(path)`: Return a `File` or `Directory` object,
  depending on the file type on disk. This function follows symlinks.

## Contributing

Clone this repo and run the tests like so:

```
npm install
npm test
```

Issues and pull requests are welcome. If you change code, be sure to re-run
`npm test`. Oftentimes it's useful to add or update tests as well.
