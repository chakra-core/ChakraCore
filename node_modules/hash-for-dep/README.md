# hash-for-dep [![Build Status](https://travis-ci.org/stefanpenner/hash-for-dep.svg?branch=stuff)](https://travis-ci.org/stefanpenner/hash-for-dep) [![Build status](https://ci.appveyor.com/api/projects/status/wf2u3j6lc52hdd21?svg=true)](https://ci.appveyor.com/project/embercli/hash-for-dep)

Generate a hash representing the stats of this module files and all its descendents files.


```js
var hashForDep = require('hash-for-dep');

hashForDep('rsvp'); // if RSVP is a dependency of the current project, you will get a checksum for it
hashForDep('rsvp', 'path/to/other/project'); //  you will get a checksum for RSVP resolved relative to the provided root
```

## What does Hash For Dep consider a dependency?

HashForDep respects the [node resolution algorithim](https://nodejs.org/api/modules.html#modules_all_together).

For example given:

```
foo/package.json
foo/index.js
foo/node_modules/a/
foo/node_modules/a/package.json
foo/node_modules/a/index.js
foo/node_modules/a/node_modules/b
foo/node_modules/a/node_modules/b/package.json
foo/node_modules/a/node_modules/b/index.js
foo/node_modules/a/node_modules/f
foo/node_modules/a/node_modules/f/index.js
foo/node_modules/a/node_modules/f/package.json
foo/node_modules/c
foo/node_modules/c/index.js
foo/node_modules/c/package.json
foo/node_modules/d
foo/node_modules/d/index.js
foo/node_modules/d/package.js
```

where `foo/package.json` depends on `a` and `c` but not `d`
and `foo/node_modules/a/package.json` depends on `b` not `f`

HashForDep will consider: `a` `c` `b` as dependencies, and simply ignore `d` and `f`.
When HashForDep considers a dependency, it will stat each of its files and those of its dependencies.


## Cache

NOTE: By default, these hashes are cached for the life of the process. As this
is the same strategy node uses for `require(x)` we can safely follow suit.

That being said, some scenarios may exist where this is not wanted. So just
like `require._cache` exists, we provide the following options:

#### To evict the cache manually (maybe for testing)

```js
require('hash-for-dep')._resetCache();
```

#### To opt out of the cache on a per invocation basis

```js
var hashForDep = require('hash-for-dep');

hashForDep(name, path, null, false /* this mysterious argument should be set to false */);
```
