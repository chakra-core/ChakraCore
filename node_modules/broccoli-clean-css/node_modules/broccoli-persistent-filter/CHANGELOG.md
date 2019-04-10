# master

# 1.2.11

* instrument walkSync times

# 1.2.10

* Add some info level logging (build summaries)

# 1.2.9

* improve logging, add some information at INFO level and not only DEBUG

# 1.2.8

* reduce package size with explicit `files` in `package.json`

# 1.2.7

* switch to heimdalljs-logger, allowing logs to show context within the broccoli
  graph

# 1.2.6

* update walk-sync, now correctly sorts directories
* update fs-tree, fixes the "rename only file in directory bug", possible performance improvements
* travis now tests against all versions of node that ember-cli supports

# 1.2.5

* remove leftover debugger
* add jshint to tests

# 1.2.4

* [logging] remove selfTime from counters

# 1.2.3

* improve debug logging, less verbose by default, but more verbose with opt-in DEBUG_VERBOSE=true

# 1.2.2

* revert FSTreeDiff update

# 1.2.1

* upgrade FSTreeDiff

# 1.2.0

* [#50](https://github.com/stefanpenner/broccoli-persistent-filter/pull/50) Add ability to return an object (must be `JSON.stringify`able) from `processString`.
* [#50](https://github.com/stefanpenner/broccoli-persistent-filter/pull/50) Add `postProcess` hook that is called after `processString` (both when cached and not cached).

# 1.0.0

* Forked from broccoli-filter
