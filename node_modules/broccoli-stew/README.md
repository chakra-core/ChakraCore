# Broccoli Stew [![Build Status](https://travis-ci.org/stefanpenner/broccoli-stew.svg)](https://travis-ci.org/stefanpenner/broccoli-stew) [![Build status](https://ci.appveyor.com/api/projects/status/orspre01ru61xiba?svg=true)](https://ci.appveyor.com/project/embercli/broccoli-stew) [![Inline docs](http://inch-ci.org/github/stefanpenner/broccoli-stew.svg?branch=master)](http://inch-ci.org/github/stefanpenner/broccoli-stew)

Provides commonly used convenience functions for developing broccoli based build pipelines.

## What's inside

Currently the following methods are available:

- [`env`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/env.js#L27) - Conditionally runs a callback based upon the current environment.
- [`mv`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/mv.js#L58) -  Moves an input tree to a different location.
- [`rename`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/rename.js#L26) - Renames files in a tree.
- [`find`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/find.js#L85) - Match files in a tree.
- [`map`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/map.js#L31) - Maps files, allow for simple content mutation.
- [`log`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/log.js#L27) - Logs out files in the passed tree.
- [`debug`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/debug.js#L12) - Writes the passed tree to disk at the root of the project.
- [`rm`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/rm.js#L40) - Remove files from a tree.
- [`afterBuild`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/after-build.js) - Calls a callback function after the tree is read.
- [`npm.main`](https://github.com/stefanpenner/broccoli-stew/blob/master/lib/npm.js#L17) - Create a tree with Node module's main entry


## Ok, but tell me more

* [using broccoli-stew to debug a broccoli tree](https://dockyard.com/blog/2015/02/02/debugging-a-broccoli-tree)
