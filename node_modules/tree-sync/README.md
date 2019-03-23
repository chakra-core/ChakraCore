# TreeSync [![Build Status](https://travis-ci.org/stefanpenner/tree-sync.svg?branch=master)](https://travis-ci.org/stefanpenner/tree-sync) [![Build status](https://ci.appveyor.com/api/projects/status/7136sbfmybx6q7w2?svg=true)](https://ci.appveyor.com/project/embercli/tree-sync)

A module for repeated efficient synchronizing two directories.

```js
// input/a/{a.js,b.js}
// output/

var tree = new TreeSync('input', 'output');
tree.sync();
// output is now contains copies of everything that is in input

fs.unlink('/input/a/b.js');

// input / output have diverged

tree.sync();

// difference is calculated and efficient patch to update `output` is created and applied
```

Under the hood, this library uses [walk-sync](https://github.com/joliss/node-walk-sync) to traverse files and folders. You may optionally pass in options to selectively include or ignore files and directories. These whitelisted properties (`ignore` and `globs`) will be passed to walk-sync.

```js
// Assume the following folder structure...
// input/
//   foo/
//     foo.js
//     bar.js
//   bar/
//     foo-bar.js

var tree = new TreeSync('input', 'output', {
  ignore: ['**/b']
});
tree.sync();

// We now expect output to contain foo/, but not bar/.
// Any changes made to bar/ won't be synced to output, and the contents of bar/ won't be traversed.
```

```js
// Assume the following folder structure...
// input/
//   foo/
//     foo.js
//     bar.js
//   bar/
//     foo-bar.js

var tree = new TreeSync('input', 'output', {
  globs: ['foo', 'foo/bar.js']
});
tree.sync();

// We now expect output to contain foo/bar.js, but nothing else.
// Be careful when using this property! You'll need to make sure that if you're including a file, it's parent
// path is also included, or you'll get an error when tree-sync tries to copy the file over.
```