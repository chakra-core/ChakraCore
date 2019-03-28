### key

Starting with v0.25.0, changelog entries will be categorized using the following labels from [keep-a-changelog][]_:

- `added`: for new features
- `changed`: for changes in existing functionality
- `deprecated`: for once-stable features removed in upcoming releases
- `removed`: for deprecated features removed in this release
- `fixed`: for any bug fixes

### [1.1.0]

**fixed**

Reverts layout changes from 1.0 to fix block-layout-nesting bug. 

There is a bug causing child blocks to be promoted up to ancestors when a nested layout/block is defined. It's not a common scenario, and probably hasn't been encountered in the wild yet since blocks were just introduced and haven't been documented yet. However, it's a bad bug, and would cause major problems if it surfaced.

The good news is that I know how to fix it. Bad news is that it will be time consuming and I need to make other changes before I get to that fix. Thus, in the meantime the best course of action is removing the blocks code.

### [1.0.0]

**Added**

- Templates now uses [dry] for handling layouts
- Advanced template-inheritance features, like `extends` and blocks! See [dry] documentation for details.

### [0.25.2]

**Fixed**

- Correctly handles arguments for the built-in singular helper when used with Handlebars.

### [0.25.1]

**Fixed**

- Ensures the template rendering engine's context is preserved.

### [0.25.0]

**Added**

- Views can now be created asynchronously by passing a callback as the last argument to `.addView` (or the method created by `.create`, e.g. `.page`)

### [0.24.3]

**Fixed**

- Ensures the `view` object has `engineStack` and `localsStack` properties

### [0.24.0]

- Bumps [base-data][] which removed `renameKey` option used when loading data. Use the `namespace` option instead.

### [0.23.0]

- Bumps [base-engine][] to fix a bug in [engine-cache][].

### [0.22.2]

- fixes `List` bug that was caused collection helpers to explode

### [0.22.0]

There should be no breaking changes in this release. If you experience a regression, please [create an issue](../../issues).

- Externalizes a few core plugins to: [base-helpers][], [base-routes][], and [base-engine][]. The goal is to allow you to use only the plugins you need in your builds.
- Improvements to lookup functions: `app.getView()` and `app.find()`
- Bumps [base][] to take advantages of code optimizations.

### [0.21.0]

**Breaking changes**

- The `queue` property has been removed, as well as related code for loading views using events. This behavior can easily be added using plugins or existing emitters.

**Non-breaking**

- The `View` and `Item` class have been externalized to modules [vinyl-item][] and [vinyl-view][] so they can be used in other libraries.

### [0.20.0]

- **Context**: In general, context should be merged so that the most specific context wins over less specific. This fixes one case where locals was winning over front-matter
- **Helpers**: Exposes `.ctx()` method on helper context, to simplify merging context in non-built-in helpers
- **Engines**: Fixes bug that was using default engine on options instead of engine that matches view file extension.

### [0.19.0]

- Numerous [dependency updates](https://github.com/jonschlinkert/templates/commit/6f78d88aa1920b84d20177bf35942e596b8e58b5)

### [0.18.0]

- [Fixes inheritance bug](https://github.com/jonschlinkert/templates/commit/66b0d885648600c97b4a158eaebf3e95443ec66e) that only manifests in node v0.4.0
- Improved [error handling in routes](https://github.com/jonschlinkert/templates/commit/d7654b74502465587da1e490c09e486fbf43f6db)

### [0.17.0]

- Removed `debug` methods and related code
- Improve layout handling with respect to template types (`partial`, `renderable` and `layout`)
- Update dependencies

### [0.16.0]

- Improved context handling
- Ensure collection middleware is handled [after app middleware](https://github.com/jonschlinkert/templates/commit/f47385f5172a2773c3ab2a969ebfccc533ec5e27)

### [0.15.0]

- removes `.removeItem` method that was deprecated in v0.10.7 from `List`
- `.handleView` is deprecated in favor of `.handleOnce` and will be removed in a future version. Start using `.handleOnce` now.
- adds a static `Templates.setup()` method for initializing any setup code that should have access to the instance before any other use code is run.
- upgrade to [base-data][] v0.4.0, which adds `app.option.set`, `app.option.get` and `app.option.merge`

### [0.14.0]

Although 99% of users won't be effected by the changes in this release, there were some **potentially breaking changes**.

- The `render` and `compile` methods were streamlined, making it clear that `.mergePartials` should not have been renamed to `mergePartialsSync`. So that change was reverted.
- Helper context: Exposes a `this.helper` object to the context in helpers, which has the helper name and options that were set specifically for that helper
- Helper context: Exposes a `this.view` object to the context in helpers, which is the current view being rendered. This was (and still is) always expose on `this.context.view`, but it makes sense to add this to the root of the context as a convenience. We will deprecate `this.context.view` in a future version.
- Helper context: `.get`, `.set` and `.merge` methods on `this.options`, `this.context` and the `this` object in helpers.

### [0.13.0]

- All template handling is async by default. Instead of adding `.compileSync`, we felt that it made more sense to add `.compileAsync`, since `.compile` is a public method and most users will expect it to be sync, and `.compile` methods with most engines are typically sync. In other words, `.compileAsync` probably won't be seen by most users, but we wanted to explain the decision to go against node.js naming conventions.
- Improved layout detection and handling

### [0.12.0]

- Adds helper methods, [.hasAsyncHelper](#hasasynchelper), [.hasHelper](#hashelper), [.getAsyncHelper](#getasynchelper), and [.getHelper](#gethelper)
- Ensures that both collection and app routes are handled when both are defined for a view

### [0.11.0]

- Default `engine` can now be defined on `app` or a collection using using `app.option('engine')`, `views.option('engine')`
- Default `layout` can now defined using `app.option('layout')`, `views.option('layout')`. No changes have been made to `view.layout`, it should work as before. Resolves [issue/#818](../../issues/818)
- Improves logic for finding a layout, this should make layouts easier to define and find going forward.
- The built-in `view` helper has been refactored completely. The helper is now async and renders the view before returning its content.
- Adds `isApp`, `isViews`, `isCollection`, `isList`, `isView`, `isGroup`, and `isItem` static methods. All return true when the given value is an instance of the respective class.
- Adds `deleteItem` method to List and Collection, and `deleteView` method to Views.
- Last, the static `_.proto` property which is only exposed for unit tests was renamed to `_.plugin`.

### [0.10.7]

- Force-update [base][] to v0.6.4 to take advantage of `isRegistered` feature.

### [0.10.6]

- Re-introduces fs logic to `getView`, now that the method has been refactored to be faster.

### [0.10.0]

- `getView` method no longer automatically reads views from the file system. This was undocumented before and, but it's a breaking change nonetheless. The removed functionality can easily be done in a plugin.

### [0.9.5]

- Fixes error messages when no engine is found for a view, and the view does not have a file extension.

### [0.9.4]

- Fixes a lookup bug in render and compile that was returning the first view that matched the given name from _any_ collection. So if a partial and a page shared the same name, if the partial was matched first it was returned. Now the `renderable` view is rendered (e.g. page)

### [0.9.0]

- _breaking change_: changes parameters on `app.context` method. It now only accepts two arguments, `view` and `locals`, since `ctx` (the parameter that was removed) was technically being merged in twice.

### [0.8.0]

- Exposes `isType` method on `view`. Shouldn't be any breaking changes.

### [0.7.0]

- _breaking change_: renamed `.error` method to `.formatError`
- adds `mergeContext` option
- collection name is now emitted with `view` and `item` as the second argument
- adds `isType` method for checking the `viewType` on a collection
- also now emits an event with the collection name when a view is created

### [0.5.1]

- fixes bug where `default` layout was automatically applied to partials, causing an infinite loop in rare cases.

[keep-a-changelog]: https://github.com/olivierlacan/keep-a-changelog
[0.5.1]: https://github.com/generate/generate/compare/0.1.0...0.5.1
