'use strict';

var fs = require('fs');
var path = require('path');
var merge = require('merge-deep');
var debug = require('../debug');
var utils = require('../utils');
var isProcessed;

/**
 * Persist a value to a namespaced config object in package.json. For
 * example, if you're using `verb`, the value would be saved to the
 * `verb` object.
 *
 * ```sh
 * # display the config
 * $ app --config
 * # set a boolean for the current project
 * $ app --config=toc
 * # save the cwd to use for the current project
 * $ app --config=cwd:foo
 * # save the tasks to run for the current project
 * $ app --config=tasks:readme
 * ```
 *
 * @name config
 * @param {Object} app
 * @api public
 * @cli public
 */

module.exports = function(app, base, options) {
  var ran = false;
  return function(val, key, config, next) {
    if (ran === true) {
      next();
      return;
    }

    ran = true;
    debug.field(key, val);

    // get the keys of properties defined by an `--init` prompt
    var keys = app.get('cache.initKeys') || [];
    var name = app._name.toLowerCase();

    if (utils.show(val)) {
      var pkgConfig = {};
      pkgConfig[name] = app.pkg.get(name) || {};
      console.error(utils.formatValue(pkgConfig));
      next();
      return;
    }

    var pkgPath = path.resolve(app.cwd, 'package.json');
    var pkg = {};

    if (utils.exists(pkgPath)) {
      pkg = JSON.parse(fs.readFileSync(pkgPath, 'utf8'));
    }

    pkg = merge({}, app.pkg.data, pkg);
    app.pkg.del(name);

    var orig = pkg[name] || {};

    // normalize both the old and new values before merging, using
    // a schema that is specifically used for normalizing values to
    // be written back to package.json
    var tmp = app.cli.schema.normalize({config: {}}) || {};
    orig = tmp.config;

    // merge the normalized values
    var merged = val;
    if (utils.isObject(val) && utils.isObject(orig)) {
      merged = merge({}, orig, val);
    }

    // show the new value in the console
    var show = utils.pick(merged, keys);
    app.pkg.logInfo('updated package.json config with', show);

    // update options and `cache.config`
    app.set('cache.config', merged);
    app.emit('config', merged);

    // update the config property
    config[key] = merged;
    if (app.pkg.queued === true) {
      app.pkg.data = JSON.parse(fs.readFileSync(pkgPath, 'utf8'));
    }

    // re-set updated config object in `package.json`
    app.pkg.set(name, merged);
    app.pkg.save();
    app.cli.process(merged, next);
  };
};
