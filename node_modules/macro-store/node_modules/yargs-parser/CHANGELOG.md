# Change Log

All notable changes to this project will be documented in this file. See [standard-version](https://github.com/conventional-changelog/standard-version) for commit guidelines.

<a name="3.2.0"></a>
# [3.2.0](https://github.com/yargs/yargs-parser/compare/v3.1.0...v3.2.0) (2016-08-13)


### Features

* coerce full array instead of each element ([#51](https://github.com/yargs/yargs-parser/issues/51)) ([cc4dc56](https://github.com/yargs/yargs-parser/commit/cc4dc56))



<a name="3.1.0"></a>
# [3.1.0](https://github.com/yargs/yargs-parser/compare/v3.0.0...v3.1.0) (2016-08-09)


### Bug Fixes

* address pkgConf parsing bug outlined in [#37](https://github.com/yargs/yargs-parser/issues/37) ([#45](https://github.com/yargs/yargs-parser/issues/45)) ([be76ee6](https://github.com/yargs/yargs-parser/commit/be76ee6))
* better parsing of negative values ([#44](https://github.com/yargs/yargs-parser/issues/44)) ([2e43692](https://github.com/yargs/yargs-parser/commit/2e43692))
* check aliases when guessing defaults for arguments fixes [#41](https://github.com/yargs/yargs-parser/issues/41) ([#43](https://github.com/yargs/yargs-parser/issues/43)) ([f3e4616](https://github.com/yargs/yargs-parser/commit/f3e4616))


### Features

* added coerce option, for providing specialized argument parsing ([#42](https://github.com/yargs/yargs-parser/issues/42)) ([7b49cd2](https://github.com/yargs/yargs-parser/commit/7b49cd2))



<a name="3.0.0"></a>
# [3.0.0](https://github.com/yargs/yargs-parser/compare/v2.4.1...v3.0.0) (2016-08-07)


### Bug Fixes

* parsing issue with numeric character in group of options ([#19](https://github.com/yargs/yargs-parser/issues/19)) ([f743236](https://github.com/yargs/yargs-parser/commit/f743236))
* upgraded lodash.assign ([5d7fdf4](https://github.com/yargs/yargs-parser/commit/5d7fdf4))

### BREAKING CHANGES

* subtle change to how values are parsed in a group of single-character arguments.
* _first released in 3.1.0, better handling of negative values should be considered a breaking change._



<a name="2.4.1"></a>
## [2.4.1](https://github.com/yargs/yargs-parser/compare/v2.4.0...v2.4.1) (2016-07-16)


### Bug Fixes

* **count:** do not increment a default value ([#39](https://github.com/yargs/yargs-parser/issues/39)) ([b04a189](https://github.com/yargs/yargs-parser/commit/b04a189))



<a name="2.4.0"></a>
# [2.4.0](https://github.com/yargs/yargs-parser/compare/v2.3.0...v2.4.0) (2016-04-11)


### Features

* **environment:** Support nested options in environment variables ([#26](https://github.com/yargs/yargs-parser/issues/26)) thanks [@elas7](https://github.com/elas7) \o/ ([020778b](https://github.com/yargs/yargs-parser/commit/020778b))



<a name="2.3.0"></a>
# [2.3.0](https://github.com/yargs/yargs-parser/compare/v2.2.0...v2.3.0) (2016-04-09)


### Bug Fixes

* **boolean:** fix for boolean options with non boolean defaults (#20) ([2dbe86b](https://github.com/yargs/yargs-parser/commit/2dbe86b)), closes [(#20](https://github.com/(/issues/20)
* **package:** remove tests from tarball ([0353c0d](https://github.com/yargs/yargs-parser/commit/0353c0d))
* **parsing:** handle calling short option with an empty string as the next value. ([a867165](https://github.com/yargs/yargs-parser/commit/a867165))
* boolean flag when next value contains the strings 'true' or 'false'. ([69941a6](https://github.com/yargs/yargs-parser/commit/69941a6))
* update dependencies; add standard-version bin for next release (#24) ([822d9d5](https://github.com/yargs/yargs-parser/commit/822d9d5))

### Features

* **configuration:** Allow to pass configuration objects to yargs-parser ([0780900](https://github.com/yargs/yargs-parser/commit/0780900))
* **normalize:** allow normalize to work with arrays ([e0eaa1a](https://github.com/yargs/yargs-parser/commit/e0eaa1a))
