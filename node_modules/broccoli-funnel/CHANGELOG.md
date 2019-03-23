# master

# 2.0.2

- Fix usage of `allowEmpty` when also using `include` / `exclude`

# 2.0.1

* Fixes issue with double dots in file names (#8) by using `fs.existSync` directly

# 2.0.0

* Drops support for `node@0.12`

# 1.2.0

* Improve excludes performance by using node-walk-sync for excludes when possible (#93)

# 1.1.0

* Opt out of cache directory creation.

# 1.0.7

* [PERF] when linking roots, only remove symlinks when absolutely needed.

# 1.0.6

* bump heimdall

# 1.0.5

* include only needed files

# 1.0.4

* switch to heimdalljs-logger, allowing logs to show context within the broccoli
  graph

# 1.0.3

* update fs-tree-diff, fixes "rename only file in directory bug", possible performance improvements.

# 1.0.2

* update minimatch

# 1.0.1

* fix annotations

# 1.0.0

* improve performance by improving output stability
* Do not use `CoreObject`
* Derive from broccoli-plugin base class

# 0.2.3

* Make `new` operator optional
* Use [new `.rebuild` API](https://github.com/broccolijs/broccoli/blob/master/docs/new-rebuild-api.md)
* Avoid mutating array arguments

# 0.2.2

No changelog beyond this point. Here be dragons.
