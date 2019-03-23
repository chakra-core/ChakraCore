# master

# 3.0.0

* Upgrade to node-merge-trees ^2.0.0; this reduces code complexity and fixes an
  obscure bug

* Bump minimum Node version to 6.0.0

# 2.0.0

* Require Node >= 4

# 1.2.4

* Revert to version 1.2.1 to restore Node 0.10 compatibility

# 1.2.3

* Bump merge-trees dependency for Node 4 compatibility

# 1.2.2

* Extract logic into node-merge-trees package

# 1.2.1

* Fix typo with cache directory opt-out.

# 1.2.0

* Opt out of cache directory creation.

# 1.1.4

* Use heimdalljs for instrumentation and logging

# 1.1.3

* Include only necessary files in npm package

# 1.1.2

* Update fs-tree-diff, fixes "rename only file in directory" bug, and possible performance improvements.

# 1.1.1

* Upgrade to can-symlink 1.0.0

# 1.1.0

* Update output directory incrementally for better performance

# 1.0.0

* Version bump without change

# 0.2.4

* Clearer error messages

# 0.2.3

* Add debug logging

# 0.2.2

* Use broccoli-plugin base class, and add `annotation` option

# 0.2.1

* Use node-symlink-or-copy to enable possible future symlink support on Windows

# 0.2.0

* Use symlinks as a performance optimization (see
  [symlink-change.md](https://github.com/broccolijs/broccoli/blob/master/docs/symlink-change.md))

# 0.1.4

* Make file overwriting case insensitive (on all platforms)

# 0.1.3

* Copy instead of hardlinking

# 0.1.2

* Update dependencies

# 0.1.1

* Use new broccoli-writer base class

# 0.1.0

* Initial version, extracted from `broccoli.MergedTree`
