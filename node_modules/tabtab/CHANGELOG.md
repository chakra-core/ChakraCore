# Change Log

All notable changes to this project will be documented in this file. See [standard-version](https://github.com/conventional-changelog/standard-version) for commit guidelines.

<a name="2.2.2"></a>
## [2.2.2](https://github.com/mklabs/node-tabtab/compare/v2.2.1...v2.2.2) (2017-01-06)


### Bug Fixes

* **win32:** fix usage of SHELL environment variable when it is not set ([9dc4380](https://github.com/mklabs/node-tabtab/commit/9dc4380))



<a name="2.2.1"></a>
## [2.2.1](https://github.com/mklabs/node-tabtab/compare/v2.2.0...v2.2.1) (2016-10-13)


### Bug Fixes

* create duplicate-free version of completion items accross evt listeners ([dc8b587](https://github.com/mklabs/node-tabtab/commit/dc8b587))



<a name="2.2.0"></a>
# [2.2.0](https://github.com/mklabs/node-tabtab/compare/v2.1.1...v2.2.0) (2016-10-11)


### Features

* **fish:** handle description by adding a tab character between name and description ([9290dcc](https://github.com/mklabs/node-tabtab/commit/9290dcc))



<a name="2.1.1"></a>
## [2.1.1](https://github.com/mklabs/node-tabtab/compare/v2.1.0...v2.1.1) (2016-10-09)


### Bug Fixes

* **zsh:** fix uninstall typo in zshrc (instead of zshhrc) ([3d29317](https://github.com/mklabs/node-tabtab/commit/3d29317))



<a name="2.1.0"></a>
# [2.1.0](https://github.com/mklabs/node-tabtab/compare/v2.0.2...v2.1.0) (2016-10-09)


### Bug Fixes

* **fish:** Disable description in fish completion per command / options ([1f04613](https://github.com/mklabs/node-tabtab/commit/1f04613))
* **fish:** fix COMP_LINE by appending a space so that prev is correctly positioned ([861f8ef](https://github.com/mklabs/node-tabtab/commit/861f8ef))


### Features

* **fish:** prevent filenames from being completed ([282b941](https://github.com/mklabs/node-tabtab/commit/282b941))



<a name="2.0.2"></a>
## [2.0.2](https://github.com/mklabs/node-tabtab/compare/v2.0.1...v2.0.2) (2016-10-06)


### Bug Fixes

* have output done after recv to handle async completion handler ([d8596ed](https://github.com/mklabs/node-tabtab/commit/d8596ed))



<a name="2.0.1"></a>
## [2.0.1](https://github.com/mklabs/node-tabtab/compare/v2.0.0...v2.0.1) (2016-10-06)


### Bug Fixes

* have uninstall command working as expected by fixing regexp ([21e2de6](https://github.com/mklabs/node-tabtab/commit/21e2de6))



<a name="1.4.2"></a>
## [1.4.2](https://github.com/mklabs/node-tabtab/compare/v1.4.1...v1.4.2) (2016-05-21)


### Bug Fixes

* **babel:** remove transform-runtime plugin ([845eb54](https://github.com/mklabs/node-tabtab/commit/845eb54))



<a name="1.4.0"></a>
# [1.4.0](https://github.com/mklabs/node-tabtab/compare/v1.3.0...v1.4.0) (2016-05-21)


### Bug Fixes

* **completion:** gather results and write only once to STDOUT ([b928bc9](https://github.com/mklabs/node-tabtab/commit/b928bc9))
* **babel:** add plugin default transform
* **feature:** Implement uninstall command and --auto flag
* **fix:** bash completion handling
* **zsh:** check for compdef


### Features

* **debug:** automatically JSON.stringify non string objects ([e4423f8](https://github.com/mklabs/node-tabtab/commit/e4423f8))
* **description:** Handle zsh description using _describe fn ([6de7ca1](https://github.com/mklabs/node-tabtab/commit/6de7ca1)), closes [#19](https://github.com/mklabs/node-tabtab/issues/19) [#21](https://github.com/mklabs/node-tabtab/issues/21)
* **uninstall:** Implement uninstall command and --auto flag ([de37993](https://github.com/mklabs/node-tabtab/commit/de37993))



<a name="1.3.2"></a>
## [1.3.2](https://github.com/mklabs/node-tabtab/compare/v1.3.0...v1.3.2) (2016-05-09)


### Bug Fixes

* **bash:** Silently fail if pkg-config bash-completion exists with non 0([0765749](https://github.com/mklabs/node-tabtab/commit/0765749))
* Skip completion install for win32 platform or unknown shell([2d0fee4](https://github.com/mklabs/node-tabtab/commit/2d0fee4))



<a name="1.3.1"></a>
## [1.3.1](https://github.com/mklabs/node-tabtab/compare/v1.3.0...v1.3.1) (2016-05-09)


### Bug Fixes

* **bash:** Silently fail if pkg-config bash-completion exits with non 0([931a746](https://github.com/mklabs/node-tabtab/commit/931a746))



<a name="1.3.0"></a>
# [1.3.0](https://github.com/mklabs/node-tabtab/compare/v1.2.1...v1.3.0) (2016-05-08)


### Features

* **cache:** Add option to enable / disable cache([7c482a4](https://github.com/mklabs/node-tabtab/commit/7c482a4)), closes [#20](https://github.com/mklabs/node-tabtab/issues/20)
* **cache:** Implement cache TTL (default: 5 min)([9157e81](https://github.com/mklabs/node-tabtab/commit/9157e81)), closes [#20](https://github.com/mklabs/node-tabtab/issues/20)



<a name="1.2.1"></a>
## [1.2.1](https://github.com/mklabs/node-tabtab/compare/v1.2.0...v1.2.1) (2016-05-08)



<a name="1.2.0"></a>
# [1.2.0](https://github.com/mklabs/node-tabtab/compare/v1.1.1...v1.2.0) (2016-05-08)


### Bug Fixes

* Use Object.assign polyfill to run on older version of node([157057a](https://github.com/mklabs/node-tabtab/commit/157057a))


### Features

* implement a basic cache mechanism([bb4216c](https://github.com/mklabs/node-tabtab/commit/bb4216c))



<a name="1.1.1"></a>
## [1.1.1](https://github.com/mklabs/node-tabtab/compare/v1.1.0...v1.1.1) (2016-05-01)


### Bug Fixes

* more generic assert on prompt ([bbcd350](https://github.com/mklabs/node-tabtab/commit/bbcd350))



<a name="1.1.0"></a>
# [1.1.0](https://github.com/mklabs/node-tabtab/compare/v1.0.5...v1.1.0) (2016-05-01)


### Bug Fixes

* **fish:** Better handling of description ([779a188](https://github.com/mklabs/node-tabtab/commit/779a188))

### Features

* **completion:** Emit completion events along package.json results ([2ed8ef5](https://github.com/mklabs/node-tabtab/commit/2ed8ef5))
* **completion:** Enhance package.json completion to support last word ([ce794d4](https://github.com/mklabs/node-tabtab/commit/ce794d4))



<a name="1.0.5"></a>
## [1.0.5](https://github.com/mklabs/node-tabtab/compare/v1.0.4...v1.0.5) (2016-04-30)




<a name="1.0.4"></a>
## [1.0.4](https://github.com/mklabs/node-tabtab/compare/v1.0.3...v1.0.4) (2016-04-30)




<a name="1.0.3"></a>
## [1.0.3](https://github.com/mklabs/node-tabtab/compare/v1.0.2...v1.0.3) (2016-04-30)


### Bug Fixes

* **babel:** Add babel as prepublish step ([97fc9ce](https://github.com/mklabs/node-tabtab/commit/97fc9ce))



<a name="1.0.2"></a>
## [1.0.2](https://github.com/mklabs/node-tabtab/compare/v1.0.1...v1.0.2) (2016-04-29)




<a name="1.0.1"></a>
## 1.0.1 (2016-04-29)

* 1.0.1 ([a2a0cff](https://github.com/mklabs/node-tabtab/commit/a2a0cff))
* zsh (on osx anyway seems to require a space before the ]] ([1f9f983](https://github.com/mklabs/node-tabtab/commit/1f9f983))
* bash: apply same spacing before closing ] ([50f0340](https://github.com/mklabs/node-tabtab/commit/50f0340))
* readme: add examples to readme ([6731663](https://github.com/mklabs/node-tabtab/commit/6731663))
* readme: fix toc ([e1d8eb1](https://github.com/mklabs/node-tabtab/commit/e1d8eb1))
* examples: add yo-complete example ([1a18381](https://github.com/mklabs/node-tabtab/commit/1a18381))
* examples: update bower-complete readme ([af9da60](https://github.com/mklabs/node-tabtab/commit/af9da60))
* fix: fix fish shell script to properly escape variables ([6f9664e](https://github.com/mklabs/node-tabtab/commit/6f9664e))



<a name="1.0.0"></a>
# 1.0.0 (2016-04-26)

* fix: check in example and fix bower-complete package.json ([c46185f](https://github.com/mklabs/node-tabtab/commit/c46185f))
* Check in examples ([82de5ef](https://github.com/mklabs/node-tabtab/commit/82de5ef))



<a name="1.0.0-pre"></a>
# 1.0.0-pre (2016-04-26)

* 1.0.0-pre ([6e15dc6](https://github.com/mklabs/node-tabtab/commit/6e15dc6))
* API example ([4c4d86c](https://github.com/mklabs/node-tabtab/commit/4c4d86c))
* badge version ([751af46](https://github.com/mklabs/node-tabtab/commit/751af46))
* Check in screenshots ([b7e3724](https://github.com/mklabs/node-tabtab/commit/b7e3724))
* Cleanup old dir ([45b09af](https://github.com/mklabs/node-tabtab/commit/45b09af))
* doc -wrong prefix ([2c8b91a](https://github.com/mklabs/node-tabtab/commit/2c8b91a))
* docs task ([305b0b4](https://github.com/mklabs/node-tabtab/commit/305b0b4))
* Ensure directory exists before writing ([bed76b3](https://github.com/mklabs/node-tabtab/commit/bed76b3))
* Event chaining, walking up the line untill it find a listener ([3c4241c](https://github.com/mklabs/node-tabtab/commit/3c4241c))
* ghpages task ([62c4362](https://github.com/mklabs/node-tabtab/commit/62c4362))
* Handle permission issue ([c44ef31](https://github.com/mklabs/node-tabtab/commit/c44ef31))
* Implement fish bridge, template system depending on $SHELL ([1823230](https://github.com/mklabs/node-tabtab/commit/1823230))
* Implement json based completion results ([5421395](https://github.com/mklabs/node-tabtab/commit/5421395))
* Init command plumbing system ([0361905](https://github.com/mklabs/node-tabtab/commit/0361905))
* init completion directory registry ([3f92281](https://github.com/mklabs/node-tabtab/commit/3f92281))
* Init v1 ([3314024](https://github.com/mklabs/node-tabtab/commit/3314024))
* install - check for existing content before writing ([2250e08](https://github.com/mklabs/node-tabtab/commit/2250e08))
* Main API and plumbing system done ([c3cba1d](https://github.com/mklabs/node-tabtab/commit/c3cba1d))
* More docs ([a483822](https://github.com/mklabs/node-tabtab/commit/a483822))
* More docs ([731222e](https://github.com/mklabs/node-tabtab/commit/731222e))
* Move old stuff ([a53de4a](https://github.com/mklabs/node-tabtab/commit/a53de4a))
* Move old stuff to .old ([94369a0](https://github.com/mklabs/node-tabtab/commit/94369a0))
* Prompt user for completion script installation method ([73f6090](https://github.com/mklabs/node-tabtab/commit/73f6090))
* readme - reformat ([be710d5](https://github.com/mklabs/node-tabtab/commit/be710d5))
* readme - update link to npm docs ([3563548](https://github.com/mklabs/node-tabtab/commit/3563548))
* rm old completion file ([cef3c00](https://github.com/mklabs/node-tabtab/commit/cef3c00))
* Shell adapters, handle bash / zsh / fish ([ab90a1a](https://github.com/mklabs/node-tabtab/commit/ab90a1a))
* Support completion item description for fish, still need work to do on zsh ([5dfc6f0](https://github.com/mklabs/node-tabtab/commit/5dfc6f0))
* test - fixing ([8ac0036](https://github.com/mklabs/node-tabtab/commit/8ac0036))
* test - fixing ([1e145d1](https://github.com/mklabs/node-tabtab/commit/1e145d1))
* test - update gentle cli ([286f803](https://github.com/mklabs/node-tabtab/commit/286f803))
* TomDocify ([9587418](https://github.com/mklabs/node-tabtab/commit/9587418))
* travis ([5fe6b73](https://github.com/mklabs/node-tabtab/commit/5fe6b73))
* travis - add babelrc file ([8a2a29b](https://github.com/mklabs/node-tabtab/commit/8a2a29b))
* travis - babel compile before test ([ddac422](https://github.com/mklabs/node-tabtab/commit/ddac422))
* travis - run mocha with babel-node ([962127c](https://github.com/mklabs/node-tabtab/commit/962127c))
* Update docs, less verbose debug output ([927e08c](https://github.com/mklabs/node-tabtab/commit/927e08c))
* update readme ([043c808](https://github.com/mklabs/node-tabtab/commit/043c808))
* wip install / uninstall ([16cdf73](https://github.com/mklabs/node-tabtab/commit/16cdf73))



<a name="0.0.4"></a>
## 0.0.4 (2015-06-06)

* :book: Fix typo ([45c6ead](https://github.com/mklabs/node-tabtab/commit/45c6ead))
* 0.0.4 ([6e6748f](https://github.com/mklabs/node-tabtab/commit/6e6748f))
* Added back new line. ([c74f7ab](https://github.com/mklabs/node-tabtab/commit/c74f7ab))
* Added default filesystem matching. ([f57a254](https://github.com/mklabs/node-tabtab/commit/f57a254))
* Didn't realize the line had {completer} before. Changing back. ([10f3472](https://github.com/mklabs/node-tabtab/commit/10f3472))
* test - update vows ([9443f42](https://github.com/mklabs/node-tabtab/commit/9443f42))
* Updated the completion script to match current npm output. ([be1c512](https://github.com/mklabs/node-tabtab/commit/be1c512))



<a name="0.0.3"></a>
## 0.0.3 (2015-01-26)

* 0.0.3 ([866546b](https://github.com/mklabs/node-tabtab/commit/866546b))
* Allow completing long options ([f847355](https://github.com/mklabs/node-tabtab/commit/f847355))
* Catching error caused by `source` closing file argument before reading from it. ([4fca6aa](https://github.com/mklabs/node-tabtab/commit/4fca6aa))
* Fix #3 - Add license info ([f3fa6c8](https://github.com/mklabs/node-tabtab/commit/f3fa6c8)), closes [#3](https://github.com/mklabs/node-tabtab/issues/3)
* rm old .pkgrc file ([42bcf50](https://github.com/mklabs/node-tabtab/commit/42bcf50))
* travis - node 0.10 ([e13de5b](https://github.com/mklabs/node-tabtab/commit/e13de5b))



<a name="0.0.2"></a>
## 0.0.2 (2012-02-08)

* add missing devDependency ([fab4faf](https://github.com/mklabs/node-tabtab/commit/fab4faf))
* bumping version ([cd56910](https://github.com/mklabs/node-tabtab/commit/cd56910))
* correct abbrev with `-` in it ([0b51ad8](https://github.com/mklabs/node-tabtab/commit/0b51ad8))
* tidy up readme a little bit, add travis file ([032d13d](https://github.com/mklabs/node-tabtab/commit/032d13d))
* tidy up the whole mess. remove unused / unnecessary code ([6a1e9c3](https://github.com/mklabs/node-tabtab/commit/6a1e9c3))



<a name="0.0.1"></a>
## 0.0.1 (2011-11-11)

* a start ([a46ca29](https://github.com/mklabs/node-tabtab/commit/a46ca29))
* add basic script for vagrant completion ([5a8fd4d](https://github.com/mklabs/node-tabtab/commit/5a8fd4d))
* add cake/rake completion, very similar ([92f125f](https://github.com/mklabs/node-tabtab/commit/92f125f))
* add completer options, decouple completed process from completer process ([c864c9d](https://github.com/mklabs/node-tabtab/commit/c864c9d))
* add gendoc script ([dbd4739](https://github.com/mklabs/node-tabtab/commit/dbd4739))
* add help module, takes a file input (md, js or cs) and man a generated manpage ([11d5d70](https://github.com/mklabs/node-tabtab/commit/11d5d70))
* add install/uninstall helper ([6cfb0ee](https://github.com/mklabs/node-tabtab/commit/6cfb0ee))
* add pkgrc help command ([fff228f](https://github.com/mklabs/node-tabtab/commit/fff228f))
* add play-complete script, completion from `play help` output ([f8347bb](https://github.com/mklabs/node-tabtab/commit/f8347bb))
* add prev to options parsed from compgen ([cfb2894](https://github.com/mklabs/node-tabtab/commit/cfb2894))
* add some commander/optimist/nopt examples script ([22e0681](https://github.com/mklabs/node-tabtab/commit/22e0681))
* add some completion install/uninstall docs ([46d324a](https://github.com/mklabs/node-tabtab/commit/46d324a))
* add vows test suite for completion output and install/uninstall cmd ([029de43](https://github.com/mklabs/node-tabtab/commit/029de43))
* completion - install instruction and simple line parsing/callback api ([ce1f1f3](https://github.com/mklabs/node-tabtab/commit/ce1f1f3))
* completion start ([94b103f](https://github.com/mklabs/node-tabtab/commit/94b103f))
* edit docs.js comments and rm lib/cli.js (was empty anyway) ([4abc675](https://github.com/mklabs/node-tabtab/commit/4abc675))
* edit package.json ([9be6eba](https://github.com/mklabs/node-tabtab/commit/9be6eba))
* initial config work, merge of global/local rc file ([64a0f7a](https://github.com/mklabs/node-tabtab/commit/64a0f7a))
* log instruction on examples when not called within completion context ([bfc6ad0](https://github.com/mklabs/node-tabtab/commit/bfc6ad0))
* move helper functions to completion module ([5fc9fa0](https://github.com/mklabs/node-tabtab/commit/5fc9fa0))
* package.json: specify directories for the docs task ([08a25ef](https://github.com/mklabs/node-tabtab/commit/08a25ef))
* parse ``` and ~~~~ special code marker in markdowns ([31ee00f](https://github.com/mklabs/node-tabtab/commit/31ee00f))
* played a little with nopt/commander options and basic completion ([c6fa6de](https://github.com/mklabs/node-tabtab/commit/c6fa6de))
* readme, npm init ([b5dae3a](https://github.com/mklabs/node-tabtab/commit/b5dae3a))
* rename to tabtab and edit test assert to use dynamic path ([061a357](https://github.com/mklabs/node-tabtab/commit/061a357))
* return warn messages as state ([8da7d5b](https://github.com/mklabs/node-tabtab/commit/8da7d5b))
* rm gendoc script ([06d3a7a](https://github.com/mklabs/node-tabtab/commit/06d3a7a))
* some docs, have more to write ([9ccd0d7](https://github.com/mklabs/node-tabtab/commit/9ccd0d7))
* Use readline's default filename completion if no matches. ([5ea2d4c](https://github.com/mklabs/node-tabtab/commit/5ea2d4c))
* warn without exiting with error, and ensure numbers on parsed env ([34a2ede](https://github.com/mklabs/node-tabtab/commit/34a2ede))
* completion: add basic abbrev support and test with nopt/commander opt ([a857dd2](https://github.com/mklabs/node-tabtab/commit/a857dd2))
* completion: add cakefile completion, testing options/tasks completion ([33c272b](https://github.com/mklabs/node-tabtab/commit/33c272b))
* completion: add optimist completion, have to parse out the help output ([6c1b1bb](https://github.com/mklabs/node-tabtab/commit/6c1b1bb))
