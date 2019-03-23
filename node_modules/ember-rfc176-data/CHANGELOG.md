# Changelog

## v0.3.8 (2019-03-14)

#### :rocket: Enhancement
* [#121](https://github.com/ember-cli/ember-rfc176-data/pull/121) Add `@action` decorator mapping. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))


## v0.3.7 (2019-02-22)

#### :rocket: Enhancement
* [#113](https://github.com/ember-cli/ember-rfc176-data/pull/113) Add `tracked` from `@glimmer/tracking`, re-export from `Ember._tracked` ([@ppcano](https://github.com/ppcano))

#### Committers: 1
- Pepe Cano ([@ppcano](https://github.com/ppcano))

## v0.3.6 (2018-12-12)

#### :rocket: Enhancement
* [#84](https://github.com/ember-cli/ember-rfc176-data/pull/84) Add notifyPropertyChange re-export from @ember/object ([@mike-north](https://github.com/mike-north))

#### Committers: 1
- Mike North ([@mike-north](https://github.com/mike-north))

## v0.3.5 (2018-09-18)

#### :rocket: Enhancement
* [#60](https://github.com/ember-cli/ember-rfc176-data/pull/60) Add `capabilities` export from `@ember/component`.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))

## v0.3.4 (2018-09-12)

#### :rocket: Enhancement
* [#43](https://github.com/ember-cli/ember-rfc176-data/pull/43) Add setComponentManager export from @ember/component. ([@smfoote](https://github.com/smfoote))

#### Committers: 1
- Steven ([smfoote](https://github.com/smfoote))

## v0.3.3 (2018-05-15)

#### :rocket: Enhancement
* [#48](https://github.com/ember-cli/ember-rfc176-data/pull/48) Add more missing public APIs. ([@rwjblue](https://github.com/rwjblue))

#### :bug: Bug Fix
* [#55](https://github.com/ember-cli/ember-rfc176-data/pull/55) Update Ember.HTMLBars.template import location. ([@rwjblue](https://github.com/rwjblue))

#### :memo: Documentation
* [#54](https://github.com/ember-cli/ember-rfc176-data/pull/54) Removed unneeded 'Module Changes' section of the README. ([@nlfurniss](https://github.com/nlfurniss))
* [#53](https://github.com/ember-cli/ember-rfc176-data/pull/53) Document mappings.json format. ([@rwjblue](https://github.com/rwjblue))
* [#50](https://github.com/ember-cli/ember-rfc176-data/pull/50) Update per-module markdown table in README. ([@rwjblue](https://github.com/rwjblue))
* [#46](https://github.com/ember-cli/ember-rfc176-data/pull/46) Update README to show helper->buildHelper. ([@bantic](https://github.com/bantic))

#### :house: Internal
* [#52](https://github.com/ember-cli/ember-rfc176-data/pull/52) Sort mappings.json by module then export... ([@rwjblue](https://github.com/rwjblue))
* [#51](https://github.com/ember-cli/ember-rfc176-data/pull/51) Add linting... ([@rwjblue](https://github.com/rwjblue))
* [#50](https://github.com/ember-cli/ember-rfc176-data/pull/50) Update per-module markdown table in README. ([@rwjblue](https://github.com/rwjblue))
* [#49](https://github.com/ember-cli/ember-rfc176-data/pull/49) Update lerna-changelog to 0.7.0. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 3
- Cory Forsyth ([bantic](https://github.com/bantic))
- Nathaniel Furniss ([nlfurniss](https://github.com/nlfurniss))
- Robert Jackson ([rwjblue](https://github.com/rwjblue))

## v0.3.2 (2018-04-02)

#### :rocket: Enhancement
* [#38](https://github.com/ember-cli/ember-rfc176-data/pull/38) Change htmlSafe and isHTMLSafe import path. ([@Serabe](https://github.com/Serabe))

#### :bug: Bug Fix
* [#45](https://github.com/ember-cli/ember-rfc176-data/pull/45) Use buildHelper as localName for Ember.Helper.helper. ([@bantic](https://github.com/bantic))

#### Committers: 2
- Cory Forsyth ([bantic](https://github.com/bantic))
- Sergio Arbeo ([Serabe](https://github.com/Serabe))

## v0.3.1 (2017-10-27)

#### :bug: Bug Fix
* [#40](https://github.com/ember-cli/ember-rfc176-data/pull/40) Update mappings for instrumentation. ([@sarupbanskota](https://github.com/sarupbanskota))

#### Committers: 1
- Sarup Banskota ([sarupbanskota](https://github.com/sarupbanskota))

## v0.3.0 (2017-10-03)

#### :rocket: Enhancement
* [#37](https://github.com/ember-cli/ember-rfc176-data/pull/37) New mappings structure. ([@Turbo87](https://github.com/Turbo87))

#### Committers: 2
- Tobias Bieniek ([Turbo87](https://github.com/Turbo87))
- [greenkeeper[bot]](https://github.com/apps/greenkeeper)

## v0.2.7 (2017-08-02)

#### :rocket: Enhancement
* [#32](https://github.com/ember-cli/ember-rfc176-data/pull/32) Expose `expandProperties`. ([@locks](https://github.com/locks))
* [#29](https://github.com/ember-cli/ember-rfc176-data/pull/29) Change "Router" default export name to "EmberRouter". ([@Turbo87](https://github.com/Turbo87))
* [#27](https://github.com/ember-cli/ember-rfc176-data/pull/27) imports should use single quotes and semicolon. ([@locks](https://github.com/locks))

#### :bug: Bug Fix
* [#28](https://github.com/ember-cli/ember-rfc176-data/pull/28) Change "Test.Adapter" default export name to "TestAdapter". ([@Turbo87](https://github.com/Turbo87))


## v0.2.6 (2017-07-24)

#### :bug: Bug Fix
* [#23](https://github.com/ember-cli/ember-rfc176-data/pull/23) Fix for Ember.String.isHTMLSafe. ([@pgrippi](https://github.com/pgrippi))


## v0.2.5 (2017-07-11)

#### :rocket: Enhancement
* [#21](https://github.com/ember-cli/ember-rfc176-data/pull/21) Add more RSVP APIs. ([@Turbo87](https://github.com/Turbo87))

#### :bug: Bug Fix
* [#22](https://github.com/ember-cli/ember-rfc176-data/pull/22) Fix "ember-components" shim mappings. ([@Turbo87](https://github.com/Turbo87))


## v0.2.4 (2017-07-08)

#### :bug: Bug Fix
* [#20](https://github.com/ember-cli/ember-rfc176-data/pull/20) Add missing APIs in Ember.Test. ([@cibernox](https://github.com/cibernox))


## v0.2.3 (2017-07-08)

#### :rocket: Enhancement
* [#18](https://github.com/ember-cli/ember-rfc176-data/pull/18) Support "import { Promise } from 'rsvp';". ([@Turbo87](https://github.com/Turbo87))

#### :bug: Bug Fix
* [#19](https://github.com/ember-cli/ember-rfc176-data/pull/19) Add missing public APIs. ([@Turbo87](https://github.com/Turbo87))
* [#17](https://github.com/ember-cli/ember-rfc176-data/pull/17) Add missing import mapping for Ember.String.isHtmlSafe(). ([@Turbo87](https://github.com/Turbo87))
* [#16](https://github.com/ember-cli/ember-rfc176-data/pull/16) Add missing import mapping for Ember.computed.uniqBy(). ([@Turbo87](https://github.com/Turbo87))


## v0.2.2 (2017-07-06)

#### :bug: Bug Fix
* [#15](https://github.com/ember-cli/ember-rfc176-data/pull/15) Remove `defineProperty` references.. ([@rwjblue](https://github.com/rwjblue))


## v0.2.1 (2017-07-06)

#### :rocket: Enhancement
* [#11](https://github.com/ember-cli/ember-rfc176-data/pull/11) Add "generate-missing-apis-list" script. ([@Turbo87](https://github.com/Turbo87))

#### :bug: Bug Fix
* [#7](https://github.com/ember-cli/ember-rfc176-data/pull/7) globals: Add missing isEqual() function. ([@Turbo87](https://github.com/Turbo87))
* [#10](https://github.com/ember-cli/ember-rfc176-data/pull/10) Add Ember.merge. ([@cibernox](https://github.com/cibernox))


## v0.2.0 (2017-07-03)

#### :rocket: Enhancement
* [#2](https://github.com/ember-cli/ember-rfc176-data/pull/2) Add old shims mapping. ([@Turbo87](https://github.com/Turbo87))


## v0.1.0 (2017-06-21)

* Initial release
