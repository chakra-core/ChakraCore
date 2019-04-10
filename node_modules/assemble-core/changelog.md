# Release History

## key

Changelog entries are classified using the following labels from [keep-a-changelog][]:

* `added`: for new features
* `changed`: for changes in existing functionality
* `deprecated`: for once-stable features removed in upcoming releases
* `removed`: for deprecated features removed in this release
* `fixed`: for any bug fixes

Custom labels used in this changelog:

* `dependencies`: bumps dependencies
* `housekeeping`: code re-organization, minor edits, or other changes that don't fit in one of the other categories.

**Heads up!**

Please [let us know](../../issues) if any of the following heading links are broken. Thanks!

## [0.31.0] - 2017-11-2011

**dependencies**

- Bumps [assemble-streams][] to ensure that `view` is decorated with `.toStream()` when created by app (instead of a collection). This is arguably a bugfix, but it might break someone's code.

## [0.30.0] - 2017-08-01

**dependencies**

- Bumps [assemble-fs][] and [assemble-render-file][] to get updates that merge the dest path information onto the context so that it can be used to calculate relative paths for navigation, pagination, etc.

## [0.29.0] - 2017-02-01

**dependencies**

- Bumps [assemble-fs] to v0.9.0 to take advantage of improvements to `.dest()` handling.

## [0.28.0] - 2017-02-01

**dependencies**

- Bumps [templates] to v1.2.0 to take advantage of new methods available on `list`s

## [0.27.0] - 2016-12-27

**dependencies**

- Bumps [templates] to v1.0.0 to take advantage of new template inheritance features!
- Bumps [assemble-fs] to v0.8.0 

## [0.26.0] - 2016-08-06

**dependencies**

- Bumps [assemble-fs][] to v0.7.0 to take advantage of `handle.once`
- Bumps [templates][] to v0.25.0 to take advantage of async collection loaders. No changes to existing API.

## [0.25.0] - 2016-07-05

**changed**

- Minor code organization and [private properties were changed](https://github.com/assemble/assemble-core/commit/5746164004647f2b7dc8883a3323922839f56958). This is technically a housekeeping change since these methods weren't exposed on the API, but it's possible someone was using them in an unintended way.

## [0.24.0]

**dependencies**

- Bumps [templates][] to v0.24.0 to use the latest [base-data][]

**removed**

- The bump in `templates` removes the `renameKey` option from the `.data` method. Use the `namespace` option instead.

## [0.23.0]

**fixed**

- Bumps [templates][] to v0.23.0 to fix bug with double rendering in [engine-cache][].

## [0.22.0]

**dependencies**

- Bumps [templates][] to v0.22.0 to take advantage of fixes and improvements to lookup methods: `.find` and `getView`. No API changes were made. Please [let us know](../../issues) if regressions occur.
- Bumps [base][] to take advantages of code optimizations.

**fixed**

- fixes `List` bug that was caused collection helpers to explode

**changed**

- Improvements to lookup functions: `app.getView()` and `app.find()`

## [0.21.0]

**dependencies**

- Bumps [templates][] to v0.21.0. 

**removed**

- Support for the `queue` property was removed on collections. See [templates][] for additional details.

## [0.20.0]

**dependencies**

- Bumps [templates][] to v0.20.0. 

**changed**

- Some changes were made to context handling that effected one unit test out of ~1,000. although it's unlikely you'll be effected by the change, it warrants a minor bump

## [0.19.0]

**dependencies**

- Bumps [templates][] to v0.19.0

**housekeeping**

- Externalizes tests (temporarily) to base-test-runner, until we get all of the tests streamlined to the same format.

## [0.18.0]

**dependencies**

- Bumps [assemble-loader][] to v0.5.0, which includes which fixes a bug where `renameKey` was not always being used when defined on collection loader options.
- Bumps [templates][] to v0.18.0, which includes fixes for resolving layouts

## [0.17.0]

**dependencies**

- Bumps [templates][] to v0.17.0

## [0.16.0]

**dependencies**

- Bumps [assemble-render-file][] to v0.5.0 and [templates][] to v0.16.0

## [0.15.0]

**dependencies**

- Bumps [assemble-streams][] to v0.5.0

## [0.14.0]

**dependencies**

- Bumps [templates][] to v0.15.1

**deprecated**

- `.handleView` method is now deprecated, use `.handleOnce` instead

**changed**

- Private method `.mergePartialsSync` rename was reverted to `.mergePartials` to be consistent with other updates in `.render` and `.compile`. 

**added**

- adds logging methods from [base-logger][] (`.log`, `.verbose`, etc)
- No other breaking changes, but some new features were added to [templates][] for handling context in views and helpers.

## [0.13.0]

- Breaking change: bumps [templates][] to v0.13.0 to fix obscure rendering bug when multiple duplicate partials were rendered in the same view. But the fix required changing the `.mergePartials` method to be async. If you're currently using `.mergePartials`, you can continue to do so synchronously using the `.mergePartialsSync` method.

## [0.9.0]

- Updates [composer][] to v0.11.0, which removes the `.watch` method in favor of using the [base-watch][] plugin.

## [0.8.0]

- Bumps several deps. [templates][] was bumped to 0.9.0 to take advantage of event handling improvements.

## [0.7.0]

- Bumps [templates][] to 0.8.0 to take advantage of `isType` method for checking a collection type, and a number of improvements to how collections and views are instantiated and named.

## [0.6.0]

- Bumps [assemble-fs][] plugin to 0.5.0, which introduces `onStream` and `preWrite` middleware handlers.
- Bumps [templates][] to 0.7.0, which fixes how non-cached collections are initialized. This was done as a minor instead of a patch since - although it's a fix - it could theoretically break someone's setup.

## [0.5.0]

- Bumps [templates][] to latest, 0.6.0, since it uses the latest [base-methods][], which introduces prototype mixins. No API changes.

## [0.4.0]

- Removed [emitter-only][] since it was only includes to be used in the default listeners that were removed in an earlier release. In rare cases this might be a breaking change, but not likely.
- Adds lazy-cache
- Updates [assemble-streams][] plugin to latest

[keep-a-changelog]: https://github.com/olivierlacan/keep-a-changelog
