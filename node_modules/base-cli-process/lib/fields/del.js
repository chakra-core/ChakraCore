'use strict';

var utils = require('../utils');

/**
 * Delete a value from `app`.
 *
 * ```json
 * # delete a value from package.json config (e.g. `verb` object)
 * $ app --del=config.foo
 * # delete a value from in-memory options
 * $ app --del=option.foo
 * # delete a property from the global config store
 * $ app --del=globals.foo
 * ```
 * @name del
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

    var keys = prop.split('.');
    var init = keys.shift();
    utils.assertPlugins(app, init);
    prop = keys.join('.');

    switch (init) {
      case 'pkg':
        app.pkg.del(prop);
        app.pkg.save();
        break;
      case 'config':
        app.pkg.del(app._name, prop);
        app.pkg.save();
        break;
      case 'globals':
        app.globals.del(prop);
        break;
      case 'store':
        app.store.del(prop);
        break;
      case 'hints':
        if (typeof app.questions !== 'undefined') {
          app.questions.hints.del(Object.keys(app.questions.hints.data));
        }
        break;
      case 'option':
        app.del(['options', prop]);
        break;
      case 'data':
        app.del(['cache.data', prop]);
        break;
      default:
        app.del(prop);
        break;
    }
    next();
  };
};
