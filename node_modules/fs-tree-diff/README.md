# fs-tree-diff [![Build Status](https://travis-ci.org/stefanpenner/fs-tree-diff.svg?branch=master)](https://travis-ci.org/stefanpenner/fs-tree-diff) [![Build status](https://ci.appveyor.com/api/projects/status/qmhx48hrquq08fam/branch/master?svg=true)](https://ci.appveyor.com/project/embercli/fs-tree-diff/branch/master)


FSTree provides the means to calculate a patch (set of operations) between one file system tree and another.

The possible operations are:

* `unlink` – remove the specified file
* `rmdir` – remove the specified folder
* `mkdir` – create the specified folder
* `create` – create the specified file
* `change` – update the specified file to reflect changes

The operations chosen aim to minimize the amount of IO required to apply a given patch.
For example, a naive `rm -rf` of a directory tree is actually quite costly, as child directories
must be recursively traversed, entries stated.. etc, all to figure out what first must be deleted.
Since we patch from tree to tree, discovering new files is both wasteful and un-needed.

The operations will also be provided in a correct order, allowing us to safely
replay operations without having to first confirm the FS is as we expect.  For
example, `unlink`s for files will occur before a `rmdir` of those files' parent
dir.  Although the ordering will be safe, a specific order is not guaranteed.

A simple example:

```js
var FSTree = require('fs-tree-diff');
var current = FSTree.fromPaths([
  'a.js'
]);

var next = FSTree.fromPaths([
  'b.js'
]);

current.calculatePatch(next) === [
  ['unlink', 'a.js', entryA],
  ['create', 'b.js', entryB]
];
```

A slightly more complicated example:

```js
var FSTree = require('fs-tree-diff');
var current = FSTree.fromPaths([
  'a.js',
  'b/',
  'b/f.js'
]);

var next = FSTree.fromPaths([
  'b.js',
  'b/',
  'b/c/',
  'b/c/d.js',
  'b/e.js'
]);

current.calculatePatch(next) === [
  ['unlink', 'a.js', entryA],
  ['create', 'b.js', entryB],
  ['mkdir', 'b/c', entryBC],
  ['create', 'b/c/d.js', entryBCD],
  ['create', 'b/e.js', entryBE]
  ['unlink', 'b/f.js', entryBF],
]
```

Now, the above examples do not demonstrate `change` operations. This is because
when providing only paths, we do not have sufficient information to check if
one entry is merely different from another with the same relativePath.

For this, FSTree supports more complex input structure. To demonstrate, we
will use the [walk-sync](https://github.com/joliss/node-walk-sync) module,
which provides higher fidelity input, allowing FSTree to also detect changes.
(See also the documentation for
[walkSync.entries](https://github.com/joliss/node-walk-sync#entries).)

```js
var walkSync = require('walk-sync');

// path/to/root/foo.js
// path/to/root/bar.js
var current = new FSTree({
  entries: walkSync.entries('path/to/root')
});

writeFileSync('path/to/root/foo.js', 'new content');
writeFileSync('path/to/root/baz.js', 'new file');

var next = new FSTree({
  entries: walkSync.entries('path/to/root')
});

current.calculatePatch(next) === [
  ['change', 'foo.js', entryFoo], // mtime + size changed, so this input is stale and needs updating.
  ['create', 'baz.js', entryBaz]  // new file, so we should create it
  /* bar stays the same and is left inert*/
];
```

The entry objects provided depend on the operation.  For `rmdir` and `unlink`
operations, the current entry is provided.  For `mkdir`, `change` and `create`
operations the new entry is provided.

## API

The public API is:

- `FSTree.fromPaths` initialize a tree from an array of string paths.
- `FSTree.fromEntries` initialize a tree from an array of `Entry` objects.
  Each entry must have the following properties (but may have more):

    - `relativePath`
    - `mode`
    - `size`
    - `mtime`
- `FSTree.applyPatch(inputDir, outputDir, patch, delegate)` applies the given
  patch from the input directory to the output directory. You can optionally
  provide a delegate object to handle individual types of patch operations.
- `FSTree.prototype.calculatePatch(newTree, isEqual)` calculate a patch against
  `newTree`.  Optionally specify a custom `isEqual` (see Change Calculation).
- `FSTree.prototype.calculateAndApplyPatch(newTree, inputDir, outputDir, delegate)`
  does a `calculatePatch` followed by `applyPatch`.
- `FSTree.prototype.addEntries(entries, options)` adds entries to an
  existing tree. Options are the same as for `FSTree.fromEntries`.
  Entries added with the same path will overwrite any existing entries.
- `FSTree.prototype.addPaths(paths, options)` adds paths to an
  existing tree. Options are the same as for `FSTree.fromPaths`.
  If entries already exist for any of the paths added, those entries will
  be updated.
- `Entry.fromStat(relativePath, stat)` creates an `Entry` from a given path and
  [`fs.Stats`](https://nodejs.org/api/fs.html#fs_class_fs_stats) object. It can
  then be used with `fromEntries` or `addEntries`.


The trees returned from `fromPaths` and `fromEntries` are relative to some base
directory.  `calculatePatch`, `applyPatch` and `calculateAndApplyPatch` all
assume that the base directory has not changed.

## Input

`FSTree.fromPaths`, `FSTree.fromEntries`, `FSTree.prototype.addPaths`,
and `FSTree.prototype.addEntries` all validate their inputs.  Inputs
must be sorted, path-unique (i.e. two entries with the same `relativePath` but
different `size`s would still be illegal input) and include intermediate
directories.

For example, the following input is **invalid**

```js
FSTree.fromPaths([
  // => missing a/ and a/b/
  'a/b/c.js'
]);
```

To have FSTree sort and expand (include intermediate directories) for you, add
the option `sortAndExpand`).

```js
FStree.fromPaths([
	'a/b/q/r/bar.js',
	'a/b/c/d/foo.js',
], { sortAndExpand: true });

// The above is equivalent to

FSTree.fromPaths([
	'a/',
	'a/b/',
	'a/b/c/',
	'a/b/c/d/',
	'a/b/c/d/foo.js',
	'a/b/q/',
	'a/b/q/r/',
	'a/b/q/r/bar.js',
]);
```

## Entry

`FSTree.fromEntries` requires you to supply your own `Entry` objects.  Your
entry objects **must** contain the following properties:

  - `relativePath`
  - `mode`
  - `size`
  - `mtime`

They must also implement the following API:

  - `isDirectory()` `true` *iff* this entry is a directory

`FSTree.fromEntries` composes well with the output of `walkSync.entries`:

```js
var walkSync = require('walk-sync');

// path/to/root/foo.js
// path/to/root/bar.js
var current = FSTree.fromEntries(walkSync.entries('path/to/root'));
```

## Change Calculation

When a prior entry has a `relativePath` that matches that of a current entry, a
change operation is included if the new entry is different from the previous
entry.  This is determined by calling `isEqual`, the optional second argument
to `calculatePatch`.  If no `isEqual` is provided, a default `isEqual` is used.

The default `isEqual` treats directories as always equal and files as different
if any of the following properties have changed.

  - `mode`
  - `size`
  - `mtime`

User specified `isEqual` will often want to use the default `isEqual`, so it is exported on `FSTree`.

Example

```js
var defaultIsEqual = FSTtreeDiff.isEqual;

function isEqualCheckingMeta(a, b) {
  return defaultIsEqual(a, b) && isMetaEqual(a, b);
}

function isMetaEqual(a, b) {
  // ...
}
```

## Patch Application

When you want to apply changes from one tree to another easily, you can use the
`FSTree.applyPatch` method. For example, given:

```js
var patch = oldInputTree.calculatePatch(newInputTree);
var inputDir = 'src';
var outputDir = 'dist';
FSTree.applyPatch(inputDir, outputDir, patch);
```

It will apply the patch changes to `dist` while using `src` as a reference for
non-destructive operations (`mkdir`, `create`, `change`). If you want to calculate
and apply a patch without any intermediate operations, you can do:

```js
var inputDir = 'src';
var outputDir = 'dist';
oldInputTree.calculateAndApplyPatch(newInputTree, inputDir, outputDir);
```

You can optionally provide a delegate object to handle applying specific types
of operations:

```js
var createCount = 0;
FSTree.applyPatch(inputDir, outputDir, patch, {
  create: function(inputPath, outputPath, relativePath) {
    createCount++;
    copy(inputPath, outputPath);
  }
});
```

The available delegate functions are the same as the supported operations:
`unlink`, `rmdir`, `mkdir`, `create`, and `change`. Each delegate function
receives the reference `inputPath`, the `outputPath`, and `relativePath` of the file
or directory for which to apply the operation.

