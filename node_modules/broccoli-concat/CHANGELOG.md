# 3.7.3

* Ensure that `CONCAT_STATS` works properly with broccoli-debug@0.6.5.

# 3.7.2

* Avoid storing larger files in memory, instead always read them from disk.

# 3.7.1

* Avoid creating concat stats folder when `process.env.CONCAT_STATS` is not set.

# 3.7.0

* Enable `process.env.CONCAT_STATS` to be set _after_ broccoli node construction (but before a build occurs).

# 3.6.0

* Support custom path for concat stats (via `process.env.CONCAT_STATS_PATH`).

# 3.5.1

* Revert changes to synchronously read larger files.

# 3.5.0

* [PERF] Synchronously read larger files.

# 3.4.1

* Remove unused code.

# 3.4.0

* Test cleanup.

# 3.3.0

* Upgrade to latest fast-sourcemap-concat version (^1.3.1).

# 3.2.0

* [Rebuild PERF] move from broccoli-caching-writer to fs-tree-diff + patch based approach (@trentmwillis)

# 3.1.1

* Fix `package.json`'s `files` array to include all files in `lib/`.

# 3.1.0

* Change base class to `broccoli-plugin` and make output persistent.

# 3.0.5

* trailing newlines after sourcemap comments, so other concat tools that don't handle that case don't have issues
* move fs-extra to dependencies (instead of devDependencies)

# 3.0.4

* [FEATURE] added CONCAT_STATS now includes more:
  * inputs of a given concat (raw files)
  * enables more advanced tooling such as: https://github.com/stefanpenner/broccoli-concat-analyser

# 3.0.3

* [FEATURE] added CONCAT_STATS env var, which enables each conat to output a summary of itself (files and sizes that where included)

# 3.0.2

* only publish files that are required for tooling

# 3.0.1

 * [BUGFIX] ensure headerFiles / inputFiles / footerFiles are included in inputFiles passed to BCW, this prevents stale reads when only a headerFile or footerFile changed but nothing else.

# 3.0.0

 * inputFiles are now sorted lexicographically, this should improve stability of output
  ember-cli wasn't following the instructions and relied on undefined (brittle) behavior.

# 2.3.2

 * [REVERT] inputFiles are now sorted lexicographically, this should improve stability of output
  ember-cli wasn't following the instructions and relied on undefined (brittle) behavior.

# 2.3.1

 * remove minimatch (unused dep)

# 2.3.0

 * inputFiles are now sorted lexicographically, this should improve stability of output

# 2.2.1

 * upgrade minimatch to ^3.0.2 (security advisory)
 * code test and repo cleanup
 * upgrade fast-sourcemap-concat
 * upgrade broccoli deps

# 2.2.0

 * update lodash
 * inputFiles parameter default to all files

# 2.1.0

 * dont mutate passed-in config
 * pass sourcemmapConfig to the underlying engine

# 2.0.4

  * ensure concat name is relevant

# 2.0.3

  * move node 5 to allowed failures
  * [BUGFIX] ensure the file handle is always cleaned-up

# 2.0.1

 * Newer fast-sourcemap-concat for upstream bugfixes.
 * Better perf due to reduced use of `stat` calls.

# 2.0.0

  * structure of output file is now as follows:
    1. header
    2. headerFiles
    3. inputFiles
    4. footerFiles
    5. footer

    Previous, 4 and 5 where reversed. This made wrapping in an IIFE needless
    complex

  * headerFiles & footerFiles now explicity throw if provided a glob entry
  * any inputFiles that also exist in headerFiles and footerFiles are now
    dropped from inputFiles


# beginning of time
