
## v7.2.0 (2019-02-25)

* If available use node workerThreads
* upgrade and cleanup dependencies

## v7.1.2 (2019-02-08)

#### :bug: Bug Fix
* [#163](https://github.com/babel/broccoli-babel-transpiler/pull/163) Prevent mismatch between worker and host process. ([@rwjblue](https://github.com/rwjblue))
* [#162](https://github.com/babel/broccoli-babel-transpiler/pull/162) Fix browser polyfill for Babel 7 ([@Elberet](https://github.com/Elberet))

#### :memo: Documentation
* [#160](https://github.com/babel/broccoli-babel-transpiler/pull/160) Add Changelog ([@Turbo87](https://github.com/Turbo87))

#### Committers: 3
- Jens Maier ([@Elberet](https://github.com/Elberet))
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))

## v7.1.1 (2018-12-06)

#### :bug: Bug Fix
* [#159](https://github.com/babel/broccoli-babel-transpiler/pull/159) parallel-api: Fix "Cannot read property '_parallelBabel' of null" error ([@Turbo87](https://github.com/Turbo87))

#### :memo: Documentation
* [#151](https://github.com/babel/broccoli-babel-transpiler/pull/151) Update README.md ([@SparshithNR](https://github.com/SparshithNR))

#### :house: Internal
* [#157](https://github.com/babel/broccoli-babel-transpiler/pull/157) TravisCI: Remove deprecated `sudo: false` option ([@Turbo87](https://github.com/Turbo87))

#### Committers: 2
- SparshithNRai ([@SparshithNR](https://github.com/SparshithNR))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))


## v7.1.0 (2018-10-29)

#### :rocket: Enhancement
* [#154](https://github.com/babel/broccoli-babel-transpiler/pull/154) Pass cachekey to worker ([@arthirm](https://github.com/arthirm))

#### Committers: 1
- Arthi ([@arthirm](https://github.com/arthirm))


## v7.0.0 (2018-08-28)

#### :boom: Breaking Change
* [#150](https://github.com/babel/broccoli-babel-transpiler/pull/150) Babel 7 ([@rwjblue](https://github.com/rwjblue))

#### :bug: Bug Fix
* [#148](https://github.com/babel/broccoli-babel-transpiler/pull/148) Deserialize nested arrays in the parallel API correctly ([@dfreeman](https://github.com/dfreeman))

#### :house: Internal
* [#149](https://github.com/babel/broccoli-babel-transpiler/pull/149) Get tests passing with Babel 7? ([@dfreeman](https://github.com/dfreeman))
* [#131](https://github.com/babel/broccoli-babel-transpiler/pull/131) CI: Use `skip_cleanup` to not revert `auto-dist-tag` adjustment ([@Turbo87](https://github.com/Turbo87))

#### Committers: 3
- Dan Freeman ([@dfreeman](https://github.com/dfreeman))
- Robert Jackson ([@rwjblue](https://github.com/rwjblue))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))


## v6.5.1 (2018-12-06)

#### :bug: Bug Fix
* [#159](https://github.com/babel/broccoli-babel-transpiler/pull/159) parallel-api: Fix "Cannot read property '_parallelBabel' of null" error ([@Turbo87](https://github.com/Turbo87))

#### Committers: 1
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))


## v6.5.0 (2018-07-24)

#### :rocket: Enhancement
* [#146](https://github.com/babel/broccoli-babel-transpiler/pull/146) Improve error ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 1
- Stefan Penner ([@stefanpenner](https://github.com/stefanpenner))


## v6.4.5 (2018-07-18)

#### :rocket: Enhancement
* [#145](https://github.com/babel/broccoli-babel-transpiler/pull/145) Enhance throwUnlessParallelizable ([@stefanpenner](https://github.com/stefanpenner))

#### :memo: Documentation
* [#143](https://github.com/babel/broccoli-babel-transpiler/pull/143) Remove 'JOBS=0' from disable parallelization section of README ([@charlespierce](https://github.com/charlespierce))

#### Committers: 2
- Charles Pierce ([@charlespierce](https://github.com/charlespierce))
- Stefan Penner ([@stefanpenner](https://github.com/stefanpenner))


## v6.4.3 (2018-05-30)

#### :bug: Bug Fix
* [#142](https://github.com/babel/broccoli-babel-transpiler/pull/142) remove debugger ([@xg-wang](https://github.com/xg-wang))

#### Committers: 1
- Thomas Wang ([@xg-wang](https://github.com/xg-wang))


## v6.4.2 (2018-05-22)

#### :house: Internal
* [#140](https://github.com/babel/broccoli-babel-transpiler/pull/140) refactor `throwUnlessParallelizable` option internals ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 1
- Stefan Penner ([@stefanpenner](https://github.com/stefanpenner))


## v6.4.0 (2018-05-21)

#### :rocket: Enhancement
* [#138](https://github.com/babel/broccoli-babel-transpiler/pull/138) Upgrade deps ([@stefanpenner](https://github.com/stefanpenner))
* [#137](https://github.com/babel/broccoli-babel-transpiler/pull/137) Add throwUnlessParallelizable (defaulting to false). ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 1
- Stefan Penner ([@stefanpenner](https://github.com/stefanpenner))


## v6.3.0 (2018-05-18)

#### :rocket: Enhancement
* [#136](https://github.com/babel/broccoli-babel-transpiler/pull/136) targetExtension improvements ([@stefanpenner](https://github.com/stefanpenner))

#### :memo: Documentation
* [#134](https://github.com/babel/broccoli-babel-transpiler/pull/134) Cleanup ([@stefanpenner](https://github.com/stefanpenner))

#### :house: Internal
* [#135](https://github.com/babel/broccoli-babel-transpiler/pull/135) be explicit about what we are importing from lib/parallel-api ([@stefanpenner](https://github.com/stefanpenner))
* [#134](https://github.com/babel/broccoli-babel-transpiler/pull/134) Cleanup ([@stefanpenner](https://github.com/stefanpenner))
* [#133](https://github.com/babel/broccoli-babel-transpiler/pull/133) Ensure logging filter for parallel stuff follows the existing convent… ([@stefanpenner](https://github.com/stefanpenner))

#### Committers: 1
- Stefan Penner ([@stefanpenner](https://github.com/stefanpenner))


## v6.2.0 (2018-05-16)

#### :rocket: Enhancement
* [#132](https://github.com/babel/broccoli-babel-transpiler/pull/132) Plugins specified as an array of serializable things can be parallelized ([@mikrostew](https://github.com/mikrostew))

#### Committers: 1
- Michael Stewart ([@mikrostew](https://github.com/mikrostew))


## v6.1.4 (2018-02-05)

#### :rocket: Enhancement
* [#126](https://github.com/babel/broccoli-babel-transpiler/pull/126) Limit to single worker pool per version of babel-core ([@mikrostew](https://github.com/mikrostew))

#### Committers: 1
- Michael Stewart ([@mikrostew](https://github.com/mikrostew))


## v6.1.3 (2018-01-31)

#### :rocket: Enhancement
* [#125](https://github.com/babel/broccoli-babel-transpiler/pull/125) Support disabling parallelism via JOBS=0 ([@stefanpenner](https://github.com/stefanpenner))

#### :bug: Bug Fix
* Module names "unknown" ([@rosshadden](https://github.com/rosshadden))

#### Committers: 2
- Ross Hadden ([@rosshadden](https://github.com/rosshadden))
- Stefan Penner ([@stefanpenner](https://github.com/stefanpenner))


## v6.1.2 (2017-08-01)

#### :rocket: Enhancement
* [#119](https://github.com/babel/broccoli-babel-transpiler/pull/119) CI: Publish tags automatically ([@Turbo87](https://github.com/Turbo87))

#### :bug: Bug Fix
* [#118](https://github.com/babel/broccoli-babel-transpiler/pull/118) package.json: Remove distTag override ([@Turbo87](https://github.com/Turbo87))

#### Committers: 2
- Michael Stewart ([@mikrostew](https://github.com/mikrostew))
- Tobias Bieniek ([@Turbo87](https://github.com/Turbo87))
