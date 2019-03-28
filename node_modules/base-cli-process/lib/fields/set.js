'use strict';

var utils = require('../utils');

/**
 * Set the given value, or a value was previously cached by `--get`.
 *
 * ```json
 * $ app --set=pkg.name:foo
 * # example: persist `pkg.name` to `store.data.name`
 * $ app --get=pkg.name --set=store.name
 * ```
 * @name set
 * @api public
 */

module.exports = function(app) {
  return function(prop, key, config, next) {
    if (typeof prop === 'undefined') {
      next();
      return;
    }

    if (utils.isObject(prop)) {
      var temp = prop;
      for (var k in temp) {
        if (temp.hasOwnProperty(k)) {
          prop = k + ':' + temp[k];
        }
      }
    }

    var cached = app.get('cache.get');
    var name, rest, val;
    var keys = [];

    if (utils.isObject(prop)) {
      keys = [];
      name = Object.keys(prop)[0];
      val = prop[name];
    } else {
      keys = String(prop).split('.');
      name = keys.shift();
      rest = keys.join('.');

      if (typeof cached !== 'undefined') {
        val = cached;
      } else {
        keys = [];
        val = {};
        val[rest] = true;
      }
    }

    utils.assertPlugins(app, name);

    switch(name) {
      case 'pkg':
        if (keys.length) {
          app.pkg.set(rest, val);
        } else {
          app.pkg.set(val);
        }
        app.pkg.save();
        break;
      case 'config':
        if (keys.length) {
          app.pkg.set([app._name, rest], val);
        } else {
          app.pkg.set(app._name, val);
        }
        app.pkg.save();
        break;
      case 'globals':
        if (keys.length) {
          app.globals.set(rest, val);
        } else {
          app.globals.set(val);
        }
        break;
      case 'store':
        if (keys.length) {
          app.store.set(rest, val);
        } else {
          app.store.set(val);
        }
        break;
      case 'option':
        if (keys.length) {
          app.option(rest, val);
        } else {
          app.option(val);
        }
        break;
      case 'data':
        if (keys.length) {
          app.data(rest, val);
        } else {
          app.data(val);
        }
        break;
      default:
        if (keys.length) {
          app.set(prop, val);
        } else {
          app.set(val);
        }
        break;
    }
    next();
  };
};
