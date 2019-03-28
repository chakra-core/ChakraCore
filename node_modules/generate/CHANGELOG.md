### Release history

#### key

Changelog entries are classified using the following labels _(from [keep-a-changelog][]_):

- `added`: for new features
- `changed`: for changes in existing functionality
- `deprecated`: for once-stable features removed in upcoming releases
- `removed`: for deprecated features removed in this release
- `fixed`: for any bug fixes
- `bumped`: updated dependencies, only minor or higher will be listed.

#### [0.13.0] - 2016-10-01

**Fixed**

- improve answer handling in `ask` listener
- update dependencies

#### [0.12.1] - 2016-09-18

**Fixed**

- Ensure `cwd` is set and `dest` is set on `app.cache.data`
- Ensure the reverse of a negative flag is set on options

#### [0.12.0] - 2016-09-12

**Added**

- Adds [update-notifier][]

#### [0.11.0] - 2016-08-17

**Fixed**

- CLI: macro handling was improved

**Removed**

- Unused `app.paths` property

**Bumped**

- [ask-when][]
- [base-questions][]
- [generate-defaults][]
- [macro-store][]

#### [0.10.0] - 2016-08-09

**Changed**

- More improvements to user-defined template handling.
- Documentation

#### [0.9.0] - 2016-07-12

**Changed**

- User-defined templates should now be stored in `~/generate/templates` instead of `~/templates`

**Added**

- [common-config][] will be used for storing user preferences. We haven't implemented any logic around this yet, but the `common-config` API is exposed on the `app.common` property, so you can begin using it in generators.


[Unreleased]: https://github.com/generate/generate/compare/0.9.0...HEAD
[0.9.0]: https://github.com/generate/generate/compare/0.8.0...0.9.0

[keep-a-changelog]: https://github.com/olivierlacan/keep-a-changelog
[common-config]: https://github.com/jonschlinkert/common-config

