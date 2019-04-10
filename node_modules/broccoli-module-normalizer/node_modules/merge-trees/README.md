# node-merge-trees

[![Build Status](https://travis-ci.org/broccolijs/node-merge-trees.svg?branch=master)](https://travis-ci.org/broccolijs/node-merge-trees)
[![Build status](https://ci.appveyor.com/api/projects/status/6c47hp9omebk09tq/branch/master?svg=true)](https://ci.appveyor.com/project/joliss/node-merge-trees/branch/master)

Symlink or copy multiple trees of files on top of each other, resulting in a single merged tree.

Optimized for repeated (incremental) merging.

## Installation

```bash
npm install --save merge-trees
```

## Usage

* `new MergeTrees(inputPaths, outputPath, options)`:

    * **`inputPaths`**: An array of paths to the input directories

    * **`outputPath`**: The path to the output directory. Must exist and be empty.

    * **`options`**: A hash of options

* `mergeTrees.merge()`: Merge the input directories into the output directory.
  Can be called repeatedly for efficient incremental merging.

### Options

* `overwrite`: By default, node-merge-trees throws an error when a file
  exists in multiple nodes. If you pass `{ overwrite: true }`, the output
  will contain the version of the file as it exists in the last input
  directory that contains it.

* `annotation`: A note to help with logging.

### Example

```js
var MergeTrees = require('merge-trees');

var mergeTrees = new MergeTrees(
  ['public', 'scripts'],
  'output-dir',
  { overwrite: true });

// Recursively symlink all files from the "public" and "scripts"
// directories into the "output-dir" directory.
mergeTrees.merge()

// ... add or remove files or directories in some input directories ...

// Incrementally update the output directory (efficient).
mergeTrees.merge()
```

Say the directory structure is as follows:

    .
    ├─ public
    │  ├─ index.html
    │  └─ images
    │     └─ logo.png
    ├─ scripts
    │  └─ app.js
    ├─ output-dir
    …

Running `mergeTrees.merge()` will generate this folder:

    .
    ├─ …
    └─ output-dir
       ├─ app.js
       ├─ index.html
       └─ images
          └─ logo.png

The parent folders, `public` and `scripts` in this case, are not included in the output. The output tree contains only the files *within* each folder, all mixed together.

## Contributing

Clone this repo and run the tests like so:

```
npm install
npm test
```

Issues and pull requests are welcome. If you change code, be sure to re-run
`npm test`. Oftentimes it's useful to add or update tests as well.
