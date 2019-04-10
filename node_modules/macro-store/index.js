/*!
 * macro-store (https://github.com/doowb/macro-store)
 *
 * Copyright (c) 2016, Brian Woodward.
 * Licensed under the MIT License.
 */

'use strict';

var utils = require('./lib/utils');
var Store = require('./lib/store');

/**
 * Handle macro processing and storing for an array of arguments.
 *
 * Set macros by specifying using the `--macro` or `--macro=set` option and a list of values.
 *
 * Remove a macro by specifying `--macro=del` option and the macro name.
 *
 * Default is to replace values in the `argv` array with stored macro values (if found).
 *
 * ```js
 * // create an argv parser
 * var parser = macros('custom-macro-store');
 *
 * // parse the argv
 * var args = parser(process.argv.slice(2));
 * // => { _: ['foo'] }
 *
 * // following input will produce the following results:
 * //
 * // Set 'foo' as ['bar', 'baz', 'bang']
 * // $ app --macro foo bar baz bang
 * // => { _: [ 'bar', 'baz', 'bang' ], macro: 'foo' }
 * //
 * // Use 'foo'
 * // $ app foo
 * // => { _: [ 'bar', 'baz', 'bang' ], isMacro: 'foo' }
 * //
 * // Remove the 'foo' macro
 * // $ app --macro --del foo
 * // => { _: [ 'foo' ], macro: 'del' }
 * ```
 *
 * @param  {String} `name` Custom name of the [data-store][] to use. Defaults to 'macros'.
 * @param  {Object} `options` Options to pass to the store to control the name or instance of the [data-store][]
 * @param  {String} `options.name` Name of the [data-store][] to use for storing macros. Defaults to `macros`
 * @param  {Object} `options.store` Instance of [data-store][] to use. Defaults to `new DataStore(options.name)`
 * @param  {Function} `options.parser` Custom argv parser to use. Defaults to [yargs-parser][]
 * @return {Function} argv parser to process macros
 * @api public
 */

module.exports = function macros(name, config) {
  if (typeof name === 'object') {
    config = name;
    name = null;
  }

  var opts = utils.extend({}, config);
  opts.name = name || opts.name;

  var store = new Store(opts);
  var parse = opts.parser || utils.parser;

  /**
   * Parser function used to parse the argv array and process macros.
   *
   * @name parser
   * @param  {Array} `argv` Array of arguments to process
   * @param  {Object} `options` Additional options to pass to the argv parser
   * @return {Object} `args` object [described above](#macros)
   * @api public
   */

  return function parser(argv, options) {
    argv = utils.arrayify(argv);

    var opts = utils.extend({}, options);
    var args = parse(argv, opts);
    var val, key;

    switch (args.macro) {
      case undefined:
        // fall through
      case 'get':
        key = args._[0];
        val = key && store.get(key);
        if (val === key) {
          val = key = null;
        }
        break;
      case 'del':
        store.del(args._[0]);
        break;
      case 'set':
        store.set(args._[0], utils.getValues(argv, 3));
        break;
      default: {
        store.set(args.macro, utils.getValues(argv, 2));
        break;
      }
    }

    var res = val ? parse(val, opts) : args;
    if (key) res.isMacro = key;
    res.__proto__ = store;
    return res;
  };
};

/**
 * Exposes `Store` for low level access
 *
 * ```js
 * var store = new macros.Store('custom-macro-store');
 * ```
 * @name .Store
 * @api public
 */

module.exports.Store = Store;
