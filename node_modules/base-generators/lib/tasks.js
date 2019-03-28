'use strict';

/**
 * ```sh
 * $ gen
 * $ gen default
 * $ gen foo
 * $ gen foo:default
 * $ gen foo,bar
 * $ gen foo bar
 * $ gen foo bar:baz
 * $ gen foo:bar,baz
 * $ gen foo.bar
 * $ gen foo.bar:baz
 * $ gen foo.bar baz
 * $ gen foo.bar baz.qux
 * $ gen foo.bar:baz qux.fez:default
 * ```
 */

// foo:bar,baz
exports.parse = function(app, name, tasks) {
  if (tasks && !tasks.length) {
    tasks = undefined;
  }

  if (Array.isArray(name)) {
    var arr = name.reduce(function(acc, val) {
      return acc.concat(exports.parse(app, val));
    }, []);

    return arr.reduce(function(acc, obj) {
      var prev = acc[acc.length - 1];
      if (prev && prev.name === obj.name) {
        prev.tasks = prev.tasks.concat(obj.tasks);
        return acc;
      }
      return acc.concat(obj);
    }, []);
  }

  if (!tasks && /:/.test(name)) {
    return exports.parse.apply(null, [app].concat(name.split(':')));
  }

  if (typeof tasks === 'string') {
    tasks = tasks.split(',');
  }

  /**
   * If `name` and `tasks` was passed, call process, e.g:
   *
   * ```sh
   * $ gen foo:default
   * $ gen foo bar:baz
   * $ gen foo:bar,baz
   * $ gen foo.bar:baz
   * $ gen foo.bar:baz qux.fez:default
   * ```
   */

  if (tasks) {
    return [{name: name, tasks: tasks}];
  }

  /**
   * Otherwise, we have one of the following:
   *
   * ```sh
   * $ gen
   * $ gen default
   * $ gen foo
   * $ gen foo,bar,baz
   * $ gen foo bar
   * $ gen foo.bar
   * $ gen foo.bar baz
   * $ gen foo.bar baz.qux
   * ```
   */

  if (typeof name === 'string') {
    name = name.split(',');
  }

  return name.reduce(function(acc, str) {
    var obj = exports.process(app, str);
    var prev = acc[acc.length - 1];
    if (prev && prev.name === obj.name) {
      prev.tasks = prev.tasks.concat(obj.tasks);
      return acc;
    }
    return acc.concat(obj);
  }, []);
};

/**
 * All we have left at this point is ambiguous args
 * that could represent tasks or generators:
 *
 * ```sh
 * $ gen
 * $ gen default
 * $ gen foo
 * $ gen foo bar
 * ```
 */

exports.process = function(app, str) {
  var generator;
  if (app.tasks.hasOwnProperty(str)) {
    return {name: app.isDefault ? 'default' : 'this', tasks: [str]};
  }

  if (/\./.test(str) || app.generators.hasOwnProperty(str)) {
    return {name: app.isDefault ? ('default.' + str) : str, tasks: ['default']};
  }

  // `app` is **not** the default generator
  if (!app.isDefault) {
    generator = app.base.getGenerator('default');
    if (generator) {
      return exports.process(generator, str);
    }
  }

  // `app` is the default generator
  generator = app.base.getGenerator(str);
  if (generator) {
    return {name: generator.name, tasks: ['default'], app: generator};
  }

  // unresolved argument
  return {_: [str]};
};
