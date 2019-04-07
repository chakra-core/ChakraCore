## v3.1.3 (2019-03-26)

#### :bug: Bug Fix
* [#106](https://github.com/ember-cli/ember-cli-version-checker/pull/106) Fix issue with PnP + missing packages ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 1
- Stefan Penner ([@stefanpenner](https://github.com/stefanpenner))

## v3.1.0 (2019-02-28)

#### :rocket: Enhancement
* [#97](https://github.com/ember-cli/ember-cli-version-checker/pull/97) Perf fix: cache the resolutions identically to how require does. ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 1
- Stefan Penner ([@stefanpenner](https://github.com/stefanpenner))

## v3.0.1 (2019-01-09)

#### :bug: Bug Fix
* [#71](https://github.com/ember-cli/ember-cli-version-checker/pull/71) fix: fallback to project if addon has no root ([@buschtoens](https://github.com/buschtoens))

#### Committers: 1
- Jan Buschtöns ([@buschtoens](https://github.com/buschtoens))


## v2.2.0 (2019-01-07)

#### :rocket: Enhancement
* [#83](https://github.com/ember-cli/ember-cli-version-checker/pull/83) Backport Yarn PnP Support to 2.x ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))


## v3.0.0 (2019-01-07)

#### :boom: Breaking Change
* [#76](https://github.com/ember-cli/ember-cli-version-checker/pull/76) Drop Node 4 support ([@Turbo87](https://github.com/Turbo87))

#### :rocket: Enhancement
* [#80](https://github.com/ember-cli/ember-cli-version-checker/pull/80) Support resolution with Yarn PnP. ([@rwjblue](https://github.com/rwjblue))

#### :memo: Documentation
* [#64](https://github.com/ember-cli/ember-cli-version-checker/pull/64) Fix code example ([@SergeAstapov](https://github.com/SergeAstapov))

#### :house: Internal
* [#82](https://github.com/ember-cli/ember-cli-version-checker/pull/82) chore(ci): install yarn via curl :weary: ([@rwjblue](https://github.com/rwjblue))
* [#81](https://github.com/ember-cli/ember-cli-version-checker/pull/81) chore(ci): Install latest yarn via apt-get ([@rwjblue](https://github.com/rwjblue))

#### Committers: 3
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))
- Sergey Astapov ([@SergeAstapov](https://github.com/SergeAstapov))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))


## v2.1.2 (2018-04-27)

#### :bug: Bug Fix
* [#53](https://github.com/ember-cli/ember-cli-version-checker/pull/53) Ensure `forEmber` _always_ uses the project. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))


## v2.1.1 (2018-04-27)

#### :bug: Bug Fix
* [#51](https://github.com/ember-cli/ember-cli-version-checker/pull/51) [bugfix] Allow VersionChecker to work with Projects ([@pzuraq](https://github.com/pzuraq))

#### :memo: Documentation
* [#49](https://github.com/ember-cli/ember-cli-version-checker/pull/49) [DOCS]: Document semver methods in README ([@alexander-alvarez](https://github.com/alexander-alvarez))

#### :house: Internal
* [#44](https://github.com/ember-cli/ember-cli-version-checker/pull/44) Linting + Prettier ([@rwjblue](https://github.com/rwjblue))

#### Committers: 3
- Alex Alvarez ([@alexander-alvarez](https://github.com/alexander-alvarez))
- Chris Garrett ([@pzuraq](https://github.com/pzuraq))
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))


## v2.1.0 (2017-10-08)

#### :rocket: Enhancement
* [#43](https://github.com/ember-cli/ember-cli-version-checker/pull/43) Add `.exists()` and document `.version`. ([@rwjblue](https://github.com/rwjblue))

#### :memo: Documentation
* [#42](https://github.com/ember-cli/ember-cli-version-checker/pull/42) Updates to newer JS syntax ([@twokul](https://github.com/twokul))

#### Committers: 2
- Alex Navasardyan ([@twokul](https://github.com/twokul))
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))


## v2.0.0 (2017-05-08)

#### :boom: Breaking Change
* [#41](https://github.com/ember-cli/ember-cli-version-checker/pull/41) Properly handle nested package resolution. ([@rwjblue](https://github.com/rwjblue))
* [#37](https://github.com/ember-cli/ember-cli-version-checker/pull/37) Refactor to Node 4 supported ES2015 syntax. ([@rwjblue](https://github.com/rwjblue))

#### :rocket: Enhancement
* [#39](https://github.com/ember-cli/ember-cli-version-checker/pull/39) Default to `npm` as type. ([@rwjblue](https://github.com/rwjblue))
* [#36](https://github.com/ember-cli/ember-cli-version-checker/pull/36)  CI: Use "auto-dist-tag" for deployment ([@Turbo87](https://github.com/Turbo87))

#### Committers: 2
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))


## v1.3.0 (2017-04-23)

#### :rocket: Enhancement
* [#32](https://github.com/ember-cli/ember-cli-version-checker/pull/32) Add support for gte(), lte(), eq() and neq() ([@Turbo87](https://github.com/Turbo87))

#### :memo: Documentation
* [#26](https://github.com/ember-cli/ember-cli-version-checker/pull/26) Add forEmber section in readme ([@josemarluedke](https://github.com/josemarluedke))

#### :house: Internal
* [#33](https://github.com/ember-cli/ember-cli-version-checker/pull/33) CI: Enable automatic NPM deployment for tags ([@Turbo87](https://github.com/Turbo87))
* [#31](https://github.com/ember-cli/ember-cli-version-checker/pull/31) Extract classes into seperate files ([@Turbo87](https://github.com/Turbo87))

#### Committers: 3
- Jared ([@coderatchet](https://github.com/coderatchet))
- Josemar Luedke ([@josemarluedke](https://github.com/josemarluedke))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))


## v1.2.0 (2016-12-04)

#### :rocket: Enhancement
* [#21](https://github.com/ember-cli/ember-cli-version-checker/pull/21) Add #forEmber helper method to get ember version from bower or npm ([@josemarluedke](https://github.com/josemarluedke))

#### :bug: Bug Fix
* [#23](https://github.com/ember-cli/ember-cli-version-checker/pull/23) All operations like `gt`, `lt`, `isAbove`, etc must return false if the dependency is not present ([@cibernox](https://github.com/cibernox))

#### Committers: 4
- Jacob Jewell ([@jakesjews](https://github.com/jakesjews))
- Josemar Luedke ([@josemarluedke](https://github.com/josemarluedke))
- Miguel Camba ([@cibernox](https://github.com/cibernox))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))


## v1.1.7 (2016-10-07)

#### :bug: Bug Fix
* [#18](https://github.com/ember-cli/ember-cli-version-checker/pull/18) Include only necessary files in the npm package ([@locks](https://github.com/locks))

#### Committers: 1
- Ricardo Mendes ([@locks](https://github.com/locks))


## v1.1.6 (2016-01-20)

#### :bug: Bug Fix
* [#14](https://github.com/ember-cli/ember-cli-version-checker/pull/14) Fix bower dependency path when in child directory ([@HeroicEric](https://github.com/HeroicEric))

#### Committers: 1
- Eric Kelly ([@HeroicEric](https://github.com/HeroicEric))


## v1.1.5 (2015-12-15)

#### :bug: Bug Fix
* [#12](https://github.com/ember-cli/ember-cli-version-checker/pull/12) Support beta/canary version number format ([@minichate](https://github.com/minichate))

#### Committers: 1
- Christopher Troup ([@minichate](https://github.com/minichate))


## 1.1.0 (2015-06-18)

#### :rocket: Enhancement
* [#3](https://github.com/ember-cli/ember-cli-version-checker/pull/3) Allow testing of other packages. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))
