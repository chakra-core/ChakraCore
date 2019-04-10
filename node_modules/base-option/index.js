/*!
 * base-option <https://github.com/node-base/base-option>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var utils = require('./utils');

module.exports = function(options) {
  return function fn(app) {
    if (!utils.isValid(app, 'base-option', '*')) return;

    var Options = utils.Options;
    var define = utils.define;
    var set = utils.set;
    var get = utils.get;

    // original constructor reference
    var ctor = this.constructor;
    Options.call(this, utils.merge(this.options, options));

    /**
     * Mixin `Options.prototype` methods
     */

    this.visit('define', Options.prototype);
    var opts = this.options;

    /**
     * Set option `key` on `app.options` with the given `value`
     * ```js
     * app.option.set('a', 'b');
     * console.log(app.option.get('a'));
     * //=> 'b'
     * ```
     * @name .option.set
     * @param {String} `key` Option key, dot-notation may be used.
     * @param {any} `value`
     * @api public
     */

    define(this.option, 'set', function(key, val) {
      set(opts, key, val);
      return opts;
    });

    /**
     * Get option `key` from `app.options`
     *
     * ```js
     * app.option({a: 'b'});
     * console.log(app.option.get('a'));
     * //=> 'b'
     * ```
     * @name .option.get
     * @param {String} `key` Option key, dot-notation may be used.
     * @return {any}
     * @api public
     */

    define(this.option, 'get', function(key) {
      return get(opts, key);
    });

    /**
     * Returns a shallow clone of `app.options` with all of the options methods, as
     * well as a `.merge` method for merging options onto the cloned object.
     *
     * ```js
     * var opts = app.option.create();
     * opts.merge({foo: 'bar'});
     * ```
     * @name .option.create
     * @param {Options} `options` Object to merge onto the returned options object.
     * @return {Object}
     * @api public
     */

    define(this.option, 'create', function(options) {
      var inst = new Options(utils.merge({}, opts));
      if (options) {
        inst.option.apply(inst, arguments);
      }

      define(inst.options, 'set', function(key, val) {
        set(this, key, val);
        return this;
      });

      define(inst.options, 'get', function(key) {
        return get(this, key);
      });

      define(inst.options, 'merge', function() {
        var args = [].concat.apply([], [].slice.call(arguments));
        args.unshift(this);
        return utils.merge.apply(utils.merge, args);
      });

      define(inst, '_callbacks', inst._callbacks);
      return inst.options;
    });

    // restore original constructor
    define(this, 'constructor', ctor);
    return fn;
  };
};
