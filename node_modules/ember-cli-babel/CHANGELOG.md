# Changelog

## v7.7.3 (2019-03-28)

#### :bug: Bug Fix
* [4b9207aa3](https://github.com/babel/ember-cli-babel/commit/4b9207aa3) Fix errors when babel-plugin-filter-imports is present ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))

## v7.7.2 (2019-03-28)

#### :bug: Bug Fix
* [#276](https://github.com/babel/ember-cli-babel/pull/276) Ensure class properties/decorator plugins are added in correct order ([@pzuraq](https://github.com/pzuraq))

#### Committers: 1
- Chris Garrett ([@pzuraq](https://github.com/pzuraq))

## v7.7.1 (2019-03-28)

#### :bug: Bug Fix
* [#275](https://github.com/babel/ember-cli-babel/pull/275) Avoid interop issues with babel-plugin-filter-imports RE: decorators syntax ([@pzuraq](https://github.com/pzuraq))

#### Committers: 1
- Chris Garrett ([@pzuraq](https://github.com/pzuraq))

## v7.7.0 (2019-03-27)

#### :rocket: Enhancement
* [#274](https://github.com/babel/ember-cli-babel/pull/274)  [FEAT] Decorator and Class Field Support ([@pzuraq](https://github.com/pzuraq))

#### Committers: 2
- Chris Garrett ([@pzuraq](https://github.com/pzuraq))
- Olle Jonsson ([@olleolleolle](https://github.com/olleolleolle))

## v7.6.0 (2019-03-14)

#### :rocket: Enhancement
* [#272](https://github.com/babel/ember-cli-babel/pull/272) Update babel-plugin-ember-modules-api-polyfill to add @action ([@rwjblue](https://github.com/rwjblue))

#### :memo: Documentation
* [#271](https://github.com/babel/ember-cli-babel/pull/271) chore: adds homepage field to pkg.json ([@jasonmit](https://github.com/jasonmit))

#### Committers: 2
- Jason Mitchell ([@jasonmit](https://github.com/jasonmit))
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))

## v7.5.0 (2019-02-22)

#### :rocket: Enhancement
* [#270](https://github.com/babel/ember-cli-babel/pull/270) Update Ember modules API to include @glimmer/tracked. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))

## v7.4.3 (2019-02-19)

#### :bug: Bug Fix
* [#265](https://github.com/babel/ember-cli-babel/pull/265) Fix `@ember/string` detection ([@locks](https://github.com/locks))

#### :house: Internal
* [#268](https://github.com/babel/ember-cli-babel/pull/268) Update babel-plugin-debug-macros ([@jrjohnson](https://github.com/jrjohnson))

#### Committers: 2
- Jonathan Johnson ([@jrjohnson](https://github.com/jrjohnson))
- Ricardo Mendes ([@locks](https://github.com/locks))

## v7.4.2 (2019-02-12)

#### :bug: Bug Fix
* [#267](https://github.com/babel/ember-cli-babel/pull/267) Ensure external helpers are not used when modules are not transpiled. ([@rwjblue](https://github.com/rwjblue))
* [#266](https://github.com/babel/ember-cli-babel/pull/266) Update to broccoli-babel-transpiler@7.1.2. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))

## v7.4.1 (2019-01-29)

#### :bug: Bug Fix
* [#264](https://github.com/babel/ember-cli-babel/pull/264) Only blacklist jQuery module when parent depends on @ember/jquery itself ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))

## v7.4.0 (2019-01-22)

#### :rocket: Enhancement
* [#261](https://github.com/babel/ember-cli-babel/pull/261) Avoid transpiling jquery module to Ember.$ if @ember/jquery is present ([@simonihmig](https://github.com/simonihmig))

#### Committers: 1
- Simon Ihmig ([@simonihmig](https://github.com/simonihmig))

## v7.3.0 (2019-01-22)

#### :rocket: Enhancement
* [#251](https://github.com/babel/ember-cli-babel/pull/251) Add ability to deduplicate babel helpers. ([@pzuraq](https://github.com/pzuraq))

#### :bug: Bug Fix
* [#260](https://github.com/babel/ember-cli-babel/pull/260) Update `@ember/string` detection to work on itself ([@locks](https://github.com/locks))

#### Committers: 2
- Chris Garrett ([@pzuraq](https://github.com/pzuraq))
- Ricardo Mendes ([@locks](https://github.com/locks))


## v7.2.0 (2018-12-12)

#### :rocket: Enhancement
* [#257](https://github.com/babel/ember-cli-babel/pull/257) Update babel-plugin-ember-modules-api-polyfill to 2.6.0. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))


## v7.1.4 (2018-12-06)

#### :rocket: Enhancement
* [#256](https://github.com/babel/ember-cli-babel/pull/256) Move `baseDir` definition into `relative-module-paths` module ([@Turbo87](https://github.com/Turbo87))
* [#255](https://github.com/babel/ember-cli-babel/pull/255) Update `amd-name-resolver` to v1.2.1 ([@Turbo87](https://github.com/Turbo87))

#### :house: Internal
* [#254](https://github.com/babel/ember-cli-babel/pull/254) TravisCI: Remove deprecated `sudo: false` option ([@Turbo87](https://github.com/Turbo87))

#### Committers: 1
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))


## v7.1.3 (2018-11-01)

#### :rocket: Enhancement
* [#249](https://github.com/babel/ember-cli-babel/pull/249) Bumping to broccoli-babel-transpiler@7.1.0 ([@arthirm](https://github.com/arthirm))

#### Committers: 1
- Arthi ([@arthirm](https://github.com/arthirm))


## v7.1.2 (2018-09-14)

#### :rocket: Enhancement
* [#244](https://github.com/babel/ember-cli-babel/pull/244) Update to ember-rfc176-data@^0.3.5. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))


## v7.1.1 (2018-09-12)

#### :rocket: Enhancement
* [#244](https://github.com/babel/ember-cli-babel/pull/244) Update to ember-rfc176-data@^0.3.4. ([@rwjblue](https://github.com/rwjblue))

#### :memo: Documentation
* [#242](https://github.com/babel/ember-cli-babel/pull/242) README: Add "Compatibility" section ([@Turbo87](https://github.com/Turbo87))

#### Committers: 2
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))


## v7.1.0 (2018-08-29)

#### :rocket: Enhancement
* [#236](https://github.com/babel/ember-cli-babel/pull/236) feat: make processed file extensions configurable ([@buschtoens](https://github.com/buschtoens))

#### Committers: 1
- Jan Buschtöns ([@buschtoens](https://github.com/buschtoens))


## v7.0.0 (2018-08-28)

#### :boom: Breaking Change
* [#140](https://github.com/babel/ember-cli-babel/pull/140) Update to use Babel 7 ([@rwjblue](https://github.com/rwjblue))
  - Drops support for Node 4.
  - Migrates to `@babel` scoped packages.
  - Drops support for ember-cli versions prior to 2.13.

#### :rocket: Enhancement
* [#140](https://github.com/babel/ember-cli-babel/pull/140) Babel 7 ([@rwjblue](https://github.com/rwjblue))

#### Committers: 2
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))
- Dan Freeman ([@dfreeman](https://github.com/dfreeman))

## v6.18.0 (2018-12-12)

#### :rocket: Enhancement
* [#257](https://github.com/babel/ember-cli-babel/pull/257) Update babel-plugin-ember-modules-api-polyfill to 2.6.0. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))

## v6.17.2 (2018-09-14)

#### :rocket: Enhancement
* [#244](https://github.com/babel/ember-cli-babel/pull/244) Update to ember-rfc176-data@^0.3.5. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))

## v6.17.0 (2018-08-27)

#### :bug: Bug Fix
* [#241](https://github.com/babel/ember-cli-babel/pull/241) Bumping broccoli-babel-transpiler to get the fix for parallelAPI ([@arthirm](https://github.com/arthirm))

#### Committers: 1
- Arthi ([@arthirm](https://github.com/arthirm))


## v6.16.0 (2018-07-18)

#### :rocket: Enhancement
* [#232](https://github.com/babel/ember-cli-babel/pull/232) Upgrade deps ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 1
- Stefan Penner ([@stefanpenner](https://github.com/stefanpenner))


## v6.15.0 (2018-07-17)

#### :rocket: Enhancement
* [#230](https://github.com/babel/ember-cli-babel/pull/230) update babel-plugin-debug-macros to 0.2.0-beta.6 ([@kellyselden](https://github.com/kellyselden))

#### :memo: Documentation
* [#225](https://github.com/babel/ember-cli-babel/pull/225) Update Changelog ([@Turbo87](https://github.com/Turbo87))

#### Committers: 2
- Kelly Selden ([@kellyselden](https://github.com/kellyselden))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))

## v6.14.1 (2018-05-26)

#### :bug: Bug Fix
* [#224](https://github.com/babel/ember-cli-babel/pull/224) Fix presets array bug. ([@twokul](https://github.com/twokul))

#### Committers: 1
- Alex Navasardyan ([twokul](https://github.com/twokul))


## v6.14.0 (2018-05-25)

#### :rocket: Enhancement
* [#222](https://github.com/babel/ember-cli-babel/pull/222) Add throwUnlessParallelizable. ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 1
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))


## v6.13.0 (2018-05-25)

#### :boom: Breaking Change
* [#223](https://github.com/babel/ember-cli-babel/pull/223) Drop Node 4 support; Avoid eagerly requiring plugins and presets. ([@mikrostew](https://github.com/mikrostew))

#### :rocket: Enhancement
* [#220](https://github.com/babel/ember-cli-babel/pull/220) upgrade deps. ([@stefanpenner](https://github.com/stefanpenner))
* [#217](https://github.com/babel/ember-cli-babel/pull/217) Bumps `broccoli-funnel` version. ([@Bartheleway](https://github.com/Bartheleway))

#### :house: Internal
* [#221](https://github.com/babel/ember-cli-babel/pull/221) Convert `ember-source-channel-url` to dev dependency. ([@stefanpenner](https://github.com/stefanpenner))
* [#219](https://github.com/babel/ember-cli-babel/pull/219) Use ES6 method syntax. ([@stefanpenner](https://github.com/stefanpenner))
* [#209](https://github.com/babel/ember-cli-babel/pull/209) CI: Add missing `skip_cleanup` flag. ([@Turbo87](https://github.com/Turbo87))

#### Committers: 4
- Barthélemy Laurans ([Bartheleway](https://github.com/Bartheleway))
- Michael Stewart ([mikrostew](https://github.com/mikrostew))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))
- Tobias Bieniek ([Turbo87](https://github.com/Turbo87))


## v6.12.0 (2018-02-27)

#### :rocket: Enhancement
* [#204](https://github.com/babel/ember-cli-babel/pull/204) add a no-op optimization. ([@kellyselden](https://github.com/kellyselden))

#### :house: Internal
* [#199](https://github.com/babel/ember-cli-babel/pull/199) Remove redundant check for targets. ([@astronomersiva](https://github.com/astronomersiva))

#### Committers: 2
- Kelly Selden ([kellyselden](https://github.com/kellyselden))
- Sivasubramanyam A ([astronomersiva](https://github.com/astronomersiva))


## v6.11.0 (2017-12-15)

#### :rocket: Enhancement
* [#197](https://github.com/babel/ember-cli-babel/pull/197) Update babel-plugin-ember-modules-api-polyfill to 2.3.0.. ([@rwjblue](https://github.com/rwjblue))

#### :memo: Documentation
* [#194](https://github.com/babel/ember-cli-babel/pull/194) Fix link. ([@jevanlingen](https://github.com/jevanlingen))

#### Committers: 2
- Jacob van Lingen ([jevanlingen](https://github.com/jevanlingen))
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.10.0 (2017-11-20)

#### :rocket: Enhancement
* [#193](https://github.com/babel/ember-cli-babel/pull/193) Allow configuration to opt-out of babel-preset-env.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))
-
## v6.9.2 (2017-11-16)

#### :bug: Bug Fix
* [#192](https://github.com/babel/ember-cli-babel/pull/192) `Project.prototype.emberCLIVersion` is a function.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))

## v6.9.1 (2017-11-16)

#### :bug: Bug Fix
* [#190](https://github.com/babel/ember-cli-babel/pull/190) Properly detect host ember-cli version.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))

## v6.9.0 (2017-11-08)

#### :rocket: Enhancement
* [#176](https://github.com/babel/ember-cli-babel/pull/176) Blacklists `@ember/string` if dependency is present. ([@locks](https://github.com/locks))

#### :memo: Documentation
* [#185](https://github.com/babel/ember-cli-babel/pull/185) Add CHANGELOG file. ([@Turbo87](https://github.com/Turbo87))

#### Committers: 3
- Alvin Crespo ([alvincrespo](https://github.com/alvincrespo))
- Ricardo Mendes ([locks](https://github.com/locks))
- Tobias Bieniek ([Turbo87](https://github.com/Turbo87))

## v6.8.2 (2017-08-30)

#### :bug: Bug Fix
* [#180](https://github.com/babel/ember-cli-babel/pull/180) Update "babel-plugin-ember-modules-api-polyfill" to v2.0.1. ([@Turbo87](https://github.com/Turbo87))

#### Committers: 1
- Tobias Bieniek ([Turbo87](https://github.com/Turbo87))


## v6.8.0 (2017-08-15)

#### :bug: Bug Fix
* [#177](https://github.com/babel/ember-cli-babel/pull/177) Update minimum version of babel-plugin-ember-modules-api-polyfill.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.7.2 (2017-08-07)

#### :rocket: Enhancement
* [#175](https://github.com/babel/ember-cli-babel/pull/175) Update `amd-name-resolver` version to enable parallel babel transpile. ([@mikrostew](https://github.com/mikrostew))

#### :house: Internal
* [#173](https://github.com/babel/ember-cli-babel/pull/173) CI: Deploy tags automatically. ([@Turbo87](https://github.com/Turbo87))

#### Committers: 2
- Michael Stewart ([mikrostew](https://github.com/mikrostew))
- Tobias Bieniek ([Turbo87](https://github.com/Turbo87))


## v6.7.1 (2017-08-02)

#### :house: Internal
* [#174](https://github.com/babel/ember-cli-babel/pull/174) update broccoli-babel-transpiler dependency to 6.1.2. ([@mikrostew](https://github.com/mikrostew))

#### Committers: 1
- Michael Stewart ([mikrostew](https://github.com/mikrostew))


## v6.7.0 (2017-08-02)

#### :rocket: Enhancement
* [#172](https://github.com/babel/ember-cli-babel/pull/172) Update "broccoli-babel-transpiler" to v6.1.1. ([@Turbo87](https://github.com/Turbo87))

#### :memo: Documentation
* [#165](https://github.com/babel/ember-cli-babel/pull/165) Include examples for where to put options. ([@dfreeman](https://github.com/dfreeman))

#### :house: Internal
* [#164](https://github.com/babel/ember-cli-babel/pull/164) Improve acceptance tests and Ember syntax. ([@villander](https://github.com/villander))
* [#167](https://github.com/babel/ember-cli-babel/pull/167) ember-cli 2.14.0 upgrade + eslint. ([@kellyselden](https://github.com/kellyselden))
* [#163](https://github.com/babel/ember-cli-babel/pull/163) Removes the flag MODEL_FACTORY_INJECTIONS. ([@villander](https://github.com/villander))

#### Committers: 4
- Dan Freeman ([dfreeman](https://github.com/dfreeman))
- Kelly Selden ([kellyselden](https://github.com/kellyselden))
- Michael Villander ([villander](https://github.com/villander))
- Tobias Bieniek ([Turbo87](https://github.com/Turbo87))


## v6.6.0 (2017-07-06)

#### :bug: Bug Fix
* [#161](https://github.com/babel/ember-cli-babel/pull/161) Avoid conflicting transforms for @ember/debug.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.5.0 (2017-07-03)

#### :rocket: Enhancement
* [#159](https://github.com/babel/ember-cli-babel/pull/159) Add emberjs/rfcs#176 support by default.. ([@rwjblue](https://github.com/rwjblue))

#### :house: Internal
* [#155](https://github.com/babel/ember-cli-babel/pull/155) only support supported versions of node. ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 2
- Robert Jackson ([rwjblue](https://github.com/rwjblue))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))


## v6.4.2 (2017-07-02)

#### :rocket: Enhancement
* [#158](https://github.com/babel/ember-cli-babel/pull/158) Respect babel sourceMaps option. ([@dwickern](https://github.com/dwickern))

#### :memo: Documentation
* [#157](https://github.com/babel/ember-cli-babel/pull/157) Add info on adding custom plugins.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 2
- Derek Wickern ([dwickern](https://github.com/dwickern))
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.4.1 (2017-06-01)

#### :rocket: Enhancement
* [#154](https://github.com/babel/ember-cli-babel/pull/154) Misc Updates. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.4.0 (2017-06-01)

#### :house: Internal
* [#145](https://github.com/babel/ember-cli-babel/pull/145) Disable loading configuration from `.babelrc`.. ([@aheuermann](https://github.com/aheuermann))
* [#152](https://github.com/babel/ember-cli-babel/pull/152) [Closes [#150](https://github.com/babel/ember-cli-babel/issues/150)] update babel-preset-env. ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 2
- Andrew Heuermann ([aheuermann](https://github.com/aheuermann))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))


## v6.2.0 (2017-05-31)

#### :rocket: Enhancement
* [#148](https://github.com/babel/ember-cli-babel/pull/148) Add node 8 support. ([@stefanpenner](https://github.com/stefanpenner))

#### :memo: Documentation
* [#139](https://github.com/babel/ember-cli-babel/pull/139) Specifically call out the `@glimmer/env` import path.. ([@rwjblue](https://github.com/rwjblue))
* [#138](https://github.com/babel/ember-cli-babel/pull/138) README: Add missing return type. ([@Turbo87](https://github.com/Turbo87))

#### :house: Internal
* [#144](https://github.com/babel/ember-cli-babel/pull/144) upgrade all the deps. ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 3
- Robert Jackson ([rwjblue](https://github.com/rwjblue))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))
- Tobias Bieniek ([Turbo87](https://github.com/Turbo87))


## v6.1.0 (2017-04-28)

#### :rocket: Enhancement
* [#133](https://github.com/babel/ember-cli-babel/pull/133) Add debug tooling babel plugins.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.0.0 (2017-04-22)

#### :boom: Breaking Change
* [#115](https://github.com/babel/ember-cli-babel/pull/115) Babel 6. ([@rwjblue](https://github.com/rwjblue))

#### :rocket: Enhancement
* [#131](https://github.com/babel/ember-cli-babel/pull/131) Expose addon powerz. ([@stefanpenner](https://github.com/stefanpenner))
* [#126](https://github.com/babel/ember-cli-babel/pull/126) Expose a public mechanism to transpile a tree.. ([@rwjblue](https://github.com/rwjblue))
* [#121](https://github.com/babel/ember-cli-babel/pull/121) Allow babel options to be passed through to babel-preset-env.. ([@rwjblue](https://github.com/rwjblue))
* [#120](https://github.com/babel/ember-cli-babel/pull/120) Expose `postTransformPlugins` to be positioned after `preset-env` plugins.. ([@rwjblue](https://github.com/rwjblue))
* [#115](https://github.com/babel/ember-cli-babel/pull/115) Babel 6. ([@rwjblue](https://github.com/rwjblue))

#### :bug: Bug Fix
* [#132](https://github.com/babel/ember-cli-babel/pull/132) Remove debugging console.log statement in index.js. ([@pgrippi](https://github.com/pgrippi))
* [#125](https://github.com/babel/ember-cli-babel/pull/125) Fix clobbering behavior with babel vs babel6 config.. ([@rwjblue](https://github.com/rwjblue))
* [#123](https://github.com/babel/ember-cli-babel/pull/123) Only pass provided options to babel-preset-env.. ([@rwjblue](https://github.com/rwjblue))
* [#122](https://github.com/babel/ember-cli-babel/pull/122) Properly forward the browsers targets to ember-preset-env. ([@kanongil](https://github.com/kanongil))
* [#117](https://github.com/babel/ember-cli-babel/pull/117) Fix issues with isPluginRequired.. ([@rwjblue](https://github.com/rwjblue))

#### :memo: Documentation
* [#128](https://github.com/babel/ember-cli-babel/pull/128) First pass at README updates for 6.. ([@rwjblue](https://github.com/rwjblue))
* [#118](https://github.com/babel/ember-cli-babel/pull/118) Add code snippet for enabling polyfill. ([@li-xinyang](https://github.com/li-xinyang))
* [#113](https://github.com/babel/ember-cli-babel/pull/113) Update README.md. ([@marpo60](https://github.com/marpo60))

#### :house: Internal
* [#124](https://github.com/babel/ember-cli-babel/pull/124) Add basic sanity test to confirm babel-preset-env is working.. ([@rwjblue](https://github.com/rwjblue))
* [#116](https://github.com/babel/ember-cli-babel/pull/116) Remove temporary fork after babel release. ([@cibernox](https://github.com/cibernox))

#### Committers: 7
- Gil Pedersen ([kanongil](https://github.com/kanongil))
- Marcelo Dominguez ([marpo60](https://github.com/marpo60))
- Miguel Camba ([cibernox](https://github.com/cibernox))
- Peter Grippi ([pgrippi](https://github.com/pgrippi))
- Robert Jackson ([rwjblue](https://github.com/rwjblue))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))
- Xinyang Li ([li-xinyang](https://github.com/li-xinyang))


## v6.0.0-beta.11 (2017-04-20)

#### :bug: Bug Fix
* [#132](https://github.com/babel/ember-cli-babel/pull/132) Remove debugging console.log statement in index.js. ([@pgrippi](https://github.com/pgrippi))

#### Committers: 1
- Peter Grippi ([pgrippi](https://github.com/pgrippi))


## v6.0.0-beta.10 (2017-04-19)

#### :rocket: Enhancement
* [#131](https://github.com/babel/ember-cli-babel/pull/131) Expose addon powerz. ([@stefanpenner](https://github.com/stefanpenner))

#### :memo: Documentation
* [#128](https://github.com/babel/ember-cli-babel/pull/128) First pass at README updates for 6.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 2
- Robert Jackson ([rwjblue](https://github.com/rwjblue))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))


## v6.0.0-beta.9 (2017-03-22)

#### :rocket: Enhancement
* [#126](https://github.com/babel/ember-cli-babel/pull/126) Expose a public mechanism to transpile a tree.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.0.0-beta.8 (2017-03-21)

#### :bug: Bug Fix
* [#125](https://github.com/babel/ember-cli-babel/pull/125) Fix clobbering behavior with babel vs babel6 config.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.0.0-beta.7 (2017-03-21)

#### :bug: Bug Fix
* [#123](https://github.com/babel/ember-cli-babel/pull/123) Only pass provided options to babel-preset-env.. ([@rwjblue](https://github.com/rwjblue))
* [#122](https://github.com/babel/ember-cli-babel/pull/122) Properly forward the browsers targets to ember-preset-env. ([@kanongil](https://github.com/kanongil))

#### :house: Internal
* [#124](https://github.com/babel/ember-cli-babel/pull/124) Add basic sanity test to confirm babel-preset-env is working.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 2
- Gil Pedersen ([kanongil](https://github.com/kanongil))
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.0.0-beta.6 (2017-03-20)

#### :rocket: Enhancement
* [#121](https://github.com/babel/ember-cli-babel/pull/121) Allow babel options to be passed through to babel-preset-env.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.0.0-beta.5 (2017-03-15)

#### :rocket: Enhancement
* [#120](https://github.com/babel/ember-cli-babel/pull/120) Expose `postTransformPlugins` to be positioned after `preset-env` plugins.. ([@rwjblue](https://github.com/rwjblue))

#### :bug: Bug Fix
* [#117](https://github.com/babel/ember-cli-babel/pull/117) Fix issues with isPluginRequired.. ([@rwjblue](https://github.com/rwjblue))

#### :memo: Documentation
* [#118](https://github.com/babel/ember-cli-babel/pull/118) Add code snippet for enabling polyfill. ([@li-xinyang](https://github.com/li-xinyang))

#### Committers: 2
- Robert Jackson ([rwjblue](https://github.com/rwjblue))
- Xinyang Li ([li-xinyang](https://github.com/li-xinyang))


## v6.0.0-beta.3 (2017-03-13)

#### :boom: Breaking Change
* [#115](https://github.com/babel/ember-cli-babel/pull/115) Babel 6. ([@rwjblue](https://github.com/rwjblue))

#### :rocket: Enhancement
* [#115](https://github.com/babel/ember-cli-babel/pull/115) Babel 6. ([@rwjblue](https://github.com/rwjblue))

#### :house: Internal
* [#116](https://github.com/babel/ember-cli-babel/pull/116) Remove temporary fork after babel release. ([@cibernox](https://github.com/cibernox))

#### Committers: 2
- Miguel Camba ([cibernox](https://github.com/cibernox))
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v6.0.0-beta.1 (2017-04-20)

#### :rocket: Enhancement
* [#131](https://github.com/babel/ember-cli-babel/pull/131) Expose addon powerz. ([@stefanpenner](https://github.com/stefanpenner))

#### :bug: Bug Fix
* [#132](https://github.com/babel/ember-cli-babel/pull/132) Remove debugging console.log statement in index.js. ([@pgrippi](https://github.com/pgrippi))

#### :memo: Documentation
* [#128](https://github.com/babel/ember-cli-babel/pull/128) First pass at README updates for 6.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 3
- Peter Grippi ([pgrippi](https://github.com/pgrippi))
- Robert Jackson ([rwjblue](https://github.com/rwjblue))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))


## v6.0.0-alpha.1 (2017-03-06)

#### :memo: Documentation
* [#113](https://github.com/babel/ember-cli-babel/pull/113) Update README.md. ([@marpo60](https://github.com/marpo60))

#### Committers: 1
- Marcelo Dominguez ([marpo60](https://github.com/marpo60))


## v5.2.4 (2017-02-09)

#### :bug: Bug Fix
* [#111](https://github.com/babel/ember-cli-babel/pull/111) Fix for undefined parent error. ([@hsdwayne](https://github.com/hsdwayne))

#### Committers: 1
- [hsdwayne](https://github.com/hsdwayne)


## v5.2.3 (2017-02-06)

#### :house: Internal
* [#110](https://github.com/babel/ember-cli-babel/pull/110) Update minimum version of broccoli-babel-transpiler.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v5.2.2 (2017-02-04)

#### :memo: Documentation
* [#107](https://github.com/babel/ember-cli-babel/pull/107) `Brocfile.js` -> `ember-cli-build.js`. ([@twokul](https://github.com/twokul))

#### :house: Internal
* [#108](https://github.com/babel/ember-cli-babel/pull/108) Add more detailed annotation.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 2
- Alex Navasardyan ([twokul](https://github.com/twokul))
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v5.2.1 (2016-12-07)

#### :bug: Bug Fix
* [#106](https://github.com/babel/ember-cli-babel/pull/106) Fix feature detection bug for setupPreprocessorRegistry.. ([@rwjblue](https://github.com/rwjblue))

#### Committers: 1
- Robert Jackson ([rwjblue](https://github.com/rwjblue))


## v5.2.0 (2016-12-07)

#### :rocket: Enhancement
* [#103](https://github.com/babel/ember-cli-babel/pull/103) Add appveyor badge. ([@stefanpenner](https://github.com/stefanpenner))
* [#100](https://github.com/babel/ember-cli-babel/pull/100) Add on example options how to disable some transformations. ([@cibernox](https://github.com/cibernox))

#### :house: Internal
* [#105](https://github.com/babel/ember-cli-babel/pull/105) Move custom options to "ember-cli-babel" options hash. ([@Turbo87](https://github.com/Turbo87))
* [#104](https://github.com/babel/ember-cli-babel/pull/104) Replace deprecated version checker method call. ([@Turbo87](https://github.com/Turbo87))
* [#101](https://github.com/babel/ember-cli-babel/pull/101) package.json cleanup. ([@Turbo87](https://github.com/Turbo87))

#### Committers: 4
- Greenkeeper ([greenkeeperio-bot](https://github.com/greenkeeperio-bot))
- Miguel Camba ([cibernox](https://github.com/cibernox))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))
- Tobias Bieniek ([Turbo87](https://github.com/Turbo87))


## v5.1.10 (2016-08-15)

#### :rocket: Enhancement
* [#89](https://github.com/babel/ember-cli-babel/pull/89) Fix issue with app.import being undefined. ([@sandersky](https://github.com/sandersky))

#### Committers: 1
- Matthew Dahl ([sandersky](https://github.com/sandersky))


## v5.1.9 (2016-08-12)

#### :rocket: Enhancement
* [#86](https://github.com/babel/ember-cli-babel/pull/86) Pass console object in to broccoli-babel-transpiler.. ([@rwjblue](https://github.com/rwjblue))
* [#81](https://github.com/babel/ember-cli-babel/pull/81) LazilyRequire broccoli-funnel. ([@stefanpenner](https://github.com/stefanpenner))
* [#78](https://github.com/babel/ember-cli-babel/pull/78) Update "ember-cli" to v1.13.8. ([@Turbo87](https://github.com/Turbo87))

#### :bug: Bug Fix
* [#88](https://github.com/babel/ember-cli-babel/pull/88) Prevent errors with console options under older ember-cli's.. ([@rwjblue](https://github.com/rwjblue))
* [#77](https://github.com/babel/ember-cli-babel/pull/77) Pin jQuery to v1.11.3 to fix builds. ([@Turbo87](https://github.com/Turbo87))

#### :house: Internal
* [#83](https://github.com/babel/ember-cli-babel/pull/83) Adds .npmignore and whitelists js files. ([@twokul](https://github.com/twokul))
* [#79](https://github.com/babel/ember-cli-babel/pull/79) upgrade ember-cli to 2.6.2. ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 8
- Alex Navasardyan ([twokul](https://github.com/twokul))
- Brian Sipple ([BrianSipple](https://github.com/BrianSipple))
- Greenkeeper ([greenkeeperio-bot](https://github.com/greenkeeperio-bot))
- Kelvin Luck ([vitch](https://github.com/vitch))
- Pat O'Callaghan ([patocallaghan](https://github.com/patocallaghan))
- Robert Jackson ([rwjblue](https://github.com/rwjblue))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))
- Tobias Bieniek ([Turbo87](https://github.com/Turbo87))


## v5.1.5 (2015-08-25)

#### :rocket: Enhancement
* [#51](https://github.com/babel/ember-cli-babel/pull/51) Using broccoli-babel-transpiler latest version. ([@msranade](https://github.com/msranade))

#### Committers: 3
- Gordon Kristan ([gordonkristan](https://github.com/gordonkristan))
- Manish Ranade ([msranade](https://github.com/msranade))
- Stefan Penner ([stefanpenner](https://github.com/stefanpenner))


## v5.1.1 (2016-08-15)

#### :rocket: Enhancement
* [#89](https://github.com/babel/ember-cli-babel/pull/89) Fix issue with app.import being undefined. ([@sandersky](https://github.com/sandersky))

#### Committers: 1
- Matthew Dahl ([sandersky](https://github.com/sandersky))
