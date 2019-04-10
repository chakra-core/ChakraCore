Version 1.13.2, released 2019-01-22
-----------------------------------
Minor improvements:
* fix for not autoescaping includes having a parent ([#606](https://github.com/twigjs/twig.js/pull/606))

Version 1.13.1, released 2019-01-19
-----------------------------------
Minor improvements:
* Fix for not autoescaping includes ([#604](https://github.com/twigjs/twig.js/pull/604))

Version 1.13.0, released 2019-01-09
-----------------------------------

Major improvements:
* Unminified sources in the npm package ([#598](https://github.com/twigjs/twig.js/pull/598))

Minor improvements:
* Multiple namespace performance improvement ([#580](https://github.com/twigjs/twig.js/pull/580))
* `|url_encode` can now extend parameters ([#588](https://github.com/twigjs/twig.js/pull/588))
* Fix `.startsWith` and `.endsWith` with `.indexOf` for IE ([#587](https://github.com/twigjs/twig.js/pull/587))
* Autoescaping improvement ([#577](https://github.com/twigjs/twig.js/pull/577))
* Support null-coalescing operator `??` ([#575](https://github.com/twigjs/twig.js/pull/575))
* Add `verbatim` tag ([#574](https://github.com/twigjs/twig.js/pull/574))
* Fix bug in `for` loop ([#573](https://github.com/twigjs/twig.js/pull/573))
* Fix twig `{% if(x) %}` and `{% elseif(x) %}` blocks parsing error ([#570](https://github.com/twigjs/twig.js/pull/570))

Version 1.12.0, released 2018-06-11
-----------------------------------

Major improvements:
* Fix array declaration on multiple lines (#546)
* Fix extend when add is null (#559)

Minor improvements:
* Improve namespaces support (#556)
* Allow express to render async (#558)

Version 1.11.1, released 2018-05-22
-----------------------------------

Major improvements:
* Upgrade to Webpack 4 (#542)
* Fix embed blocks logic (#537)

Minor improvements:
* Improve detection of when a block is in a loop (#541)
* Add possibility to set default value for a macro parameter (#544)

Version 1.11.0, released 2018-04-10
-----------------------------------

Major improvements:
* Add support for 'with' tag (#497)
* Add date support for json_encode filter (#515)
* Fix 'embed' tag options (#534)
* Performance improvements when including templates (#492)

Minor improvements:
* Fix incorrect 'and' and 'or' behaviour when comparing variables (#481)
* Remove 'trim' filter autoescape (#488)
* Fix empty output from loop with async call (#538)
* Add allowable tags to strip tags filter (#524)


Version 1.10.5, released 2017-05-24
-----------------------------------

Minor improvements:
* Template id is now returned as part of the error when an exception is thrown (#464)
* Test result no longer dependent on the name of the test file (#465)
* Fix unexpected 'const' (#471)

Version 1.10.4, released 2017-03-02
-----------------------------------

Minor improvements:
* Fixed missing changelog updates

Version 1.10.3, released 2017-03-02
-----------------------------------

Major improvements:
* Async rendering and filters (#457)
* From aliases (#438)
* Bitwise operators (#443)
* Fix object method context (#455)

Minor improvements:
* Readme updates (#454)
* 'not' unary can be more widely used (#444)
* Fix `importFile` relative path handling (#449)

Version 0.10.3, released 2016-12-09
-----------------------------------
Minor improvements:
* Spaceless tag no longer escapes Static values (#435)
* Inline includes now load (#433)
* Errors during async fs loading use error callback (#431)

Version 0.10.2, released 2016-11-23
-----------------------------------
Minor improvements:
* Support 'same as' (#429)
* Fix windows colon colon namespace (#430)

Version 0.10.1, released 2016-11-18
-----------------------------------

Minor improvements:
* Fixed missing changelog updates
* Fixed incorrect versions in source
* Rethrow errors when option to do so is set (#422)

Version 0.10.0, released 2016-10-28
-----------------------------------
Bower is no longer supported

Major improvements:
* Updated to locutus which replaces phpjs
* elseif now accepts truthy values (#370)
* Use PHP style falsy matching (#383)
* Fix 'not' after binary expressions (#385)
* Use current context when parsing an include (#395)
* Correct handling of 'ignore missing' in embed and include (#424)

Minor improvements:
* Documentation updates
* Refreshed dependencies

Version 0.9.5, released 2016-05-14
-----------------------------------

Minor improvements:
* Templates that are included via "extends" now populate the parent template context

Version 0.9.4, released 2016-05-13
-----------------------------------
Parentheses parsing has undergone quite a large refactoring, but nothing should have explicitly broken.

Major improvements:
* Subexpressions are now supported and parsed differently from function parameters

Version 0.9.3, released 2016-05-12
-----------------------------------
Fix missing changelog updates

Version 0.9.2, released 2016-05-12
-----------------------------------
Minor improvements:
* Empty strings can now be passed to the date filter
* Twig.expression.resolve keeps the correct context for `this`

Version 0.9.1, released 2016-05-10
-----------------------------------
Fixed changelog versioning

Version 0.9.0, released 2016-05-10
-----------------------------------
Theoretically no breaking changes, but lots of things have changed so it is possible something has slipped through.

Dependencies have been updated. You should run `npm install` to update these.

Major improvements:
* Webpack is now used for builds
* phpjs is now a dependency and replaces our reimplementation of PHP functions (#343)
* Arrays are now cast to booleans unless accessing their contents
* in/not in operator precedence changed (#344)
* Expressions can now be keys (#350)
* The extended ternary operator is now supported (#354)
* Expressions can now appear after period accessor (#356)
* The slice shorthand is now supported (#362)

Minor improvements:
* Twig.exports.renderFile now returns a string rather than a String (#348)
* The value of context is now cloned when setting a variable to context (#345)
* Negative numbers are now correctly parsed (#353)
* The // operator now works correctly (#353)


Version 0.8.9, released 2016-03-18
-----------------------------
Dependencies have been updated to current versions. You should run `npm install` to update these. (#313)

Major improvements:
* Twig's `source` function is now supported (#309)
* It is possible to add additional parsers using Twig.Templates.registerParser() (currently available: twig, source). If you are using a custom loader, please investigate src/twig.loader.fs.js how to call the requested parser. (#309)
* `undefined` and `null` values now supported in the `in` operator (#311)
* Namespaces can now be defined using the '@' symbol (#328)

Minor improvements:
* Undefined object properties now have the value of `undefined` rather than `null` (#311)
* Improved browser tests (#325, #310)
* IE8 fix (#324)
* Path resolution has been refactored to its own module (#323)

Version 0.8.8, released 2016-02-13
----------------------------------
Major improvements:
* Support for [block shortcuts](http://twig.sensiolabs.org/doc/tags/extends.html#block-shortcuts): `{% block title page_title|title %}` (#304)
* Define custom template loaders, by registering them via `Twig.Templates.registerLoader()` (#301)

Minor improvements:
* Some mocha tests didn't  work in browsers (#281)
* Fix Twig.renderFile (#303)

[All issues of this milestone](https://github.com/justjohn/twig.js/issues?q=milestone%3A0.8.8)

Version 0.8.7, released 2016-01-20
----------------------------------
Major improvements:
* The `autoescape` option now supports all strategies which are supported by the `escape` filter (#299)

Minor improvements:
* The `date` filter now recognises unix timestamps as input, when they are passed as string (#296)
* The `default` filter now allows to be called without parameters (it will return an empty string in that case) (#295)
* Normalize provided template paths (this generated problems when using nodejs under Windows) (#252, #300)

Version 0.8.6, released 2016-01-05
----------------------------------
Major improvements:
* The `escape` filter now supports the strategy parameter: `{{ var|escape('css') }}` with the following available strategies: html (default), js, css, url, html_attr. (#289)

Minor improvements:
* The filter `url_encode` now also encodes apostrophe (as in Twig.php) (#288)
* Minor bugfixes (#290, #291)

Version 0.8.5, released 2015-12-24
----------------------------------
From 0.8.5 on, a summary of changes between each version will be included in the CHANGELOG.md file.

There were some changes to the [Contribution guidelines](https://github.com/justjohn/twig.js/wiki/Contributing): please commit only changes to source files, the files `twig.js` and `twig.min.js` will be rebuilt when a new version gets released. Therefore you need to run `make` after cloning resp. pulling (if you want to use the development version).

Major improvements:
* Implement `min` and `max` functions (#164)
* Support for the whitespace control modifier: `{{- -}}` (#266)
* `sort` filter: try to cast values to match type (numeric values to number, string otherwise) (#278)
* Support for twig namespaces (#195, #251)
* Support for expressions as object keys: `{% set foo = { (1 + 1): 'bar' } %}` (#284)

Minor improvements:
* Allow integer 0 as key in objects: `{ 0: "value" }` (#186)
* `json_encode` filter: always return objects in order of keys, also ignore the internal key `_keys` for nested objects (#279)
* `date` filter: update to current strtotime() function from phpjs: now support ISO8601 dates as input on Mozilla Firefox. (#276)
* Validate template IDs only when caching is enabled (#233, #259)
* Support xmlhttp.status==0 when using cordova (#240)
* Improved sub template file loading (#264)
* Ignore quotes between `{% raw %}` and `{% endraw %}` (#286)
