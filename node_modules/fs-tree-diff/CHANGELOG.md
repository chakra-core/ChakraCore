# master

# 1.0.2

* fix type module export thanks @dfreeman

# 1.0.1

* fix errors in type definitions thanks @dfreeman

# 1.0.0

* no changes

# v0.5.9

* publish typescript types thanks @amiller-gh

# v0.5.8

* add typescript types thanks @amiller-gh

# v0.5.7

* avoid errors when attempting to unlink a file that has already been removed.

# v0.5.5

* add `Entry.fromStat` thanks @trentmwillis
* add `applyPatch` and `calculateAndApplyPatch` thanks @trentmwillis

# v0.5.4

* Fix remove-before-add bug.  Thanks @dfreeman for excellent bug investigation

# v0.5.3

* Add `FSTree.prototype.addEntries` thanks @chriseppstein

# v0.5.2

* bump version of heimdalljs-logger

# v0.5.1

* replace direct use of debug with heimdalljs-logger

# v0.5.0

* [BREAKING] drop `unlinkdir` and `linkDir` as operations.  Downstream can
  implement this by examining entries, eg by marking them beforehand as
  broccoli-merge-trees does.
* [BREAKING] `unlink` and `rmdir` operations are now passed the entry
* [BREAKING] entries must be lexigraphicaly sorted by relative path.  To do this
  implicitly, use `sortAndExpand`.
* [BREAKING] entries must include intermediate directories.  To do this
  implicitly, use `sortAndExpand`.
* reworked implementation to diff via two sorted arrays
* performance improvements
* return entires as-provided, preserving user-specified metadata
* directories in patches always end with a trailing slash
* fixes various issues related to directory state transitions
* directories can now receive `change` patches if user-supplied `meta` has
  property changes

# v0.4.4

* throw errors on duplicate entries (previous behavior was unspecified)

# v0.4.2

* coerce time to number before comparison

# v0.4.1

* add `:` in debug namespace for ecosystem consistency

# v0.4.0

* initial release
