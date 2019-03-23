# node-clean-up-path

[![Build Status](https://travis-ci.org/broccolijs/node-clean-up-path.svg?branch=master)](https://travis-ci.org/broccolijs/node-clean-up-path)

## Installation

```bash
npm install --save clean-up-path
```

## Usage

```js
let cleanUpPath = require("clean-up-path");

let absolutePath = cleanUpPath(somePath);

// Now safely use p as a symlink target, etc.
```

`cleanUpPath(p)` is similar to
[`path.resolve(p)`](https://nodejs.org/docs/latest/api/path.html#path_path_resolve_paths),
but it is faster for paths that are already resolved, and on Windows it turns
LFS paths (`c:\foo`) into long UNC paths (`\\?\c:\foo`) to deal with path length
limitations.

Unlike `path.resolve`, it does not guarantee that the returned path contains no
`..` or `.` segments.

## Contributing

Clone this repo and run the tests like so:

```
npm install
npm test
```

Issues and pull requests are welcome. If you change code, be sure to re-run
`npm test`. Oftentimes it's useful to add or update tests as well.
