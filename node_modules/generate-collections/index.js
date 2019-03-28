'use strict';

var fs = require('fs');
var os = require('os');
var path = require('path');
var utils = require('./utils');

/**
 * Register the plugin.
 *
 * ```js
 * // in your generator
 * this.use(require('generate-collections'));
 *
 * // or, to customize options
 * var collections = require('generate-collections');
 * this.use(collections.create([options]));
 * ```
 * @api public
 */

function collections(config) {
  config = config || {};

  return function plugin(app, base) {
    if (!utils.isValid(app, 'generate-collections')) return;
    app.define('home', path.resolve.bind(path, os.homedir()));

    /**
     * Options
     */

    app.option(config);

    // add default view collections
    if (!app.files) app.create('files', { viewType: 'renderable'});
    if (!app.includes) app.create('includes', { viewType: 'partial' });
    if (!app.layouts) app.create('layouts', { viewType: 'layout' });
    if (!app.templates) app.create('templates', { viewType: 'renderable' });

    /**
     * Middleware for collections created by this generator
     */

    app.preLayout(/./, function(view, next) {
      if (utils.falsey(view.layout) && !view.isType('partial')) {
        // use the empty layout created above, to ensure that all
        // pre-and post-layout middleware are still triggered
        view.layout = app.resolveLayout(view);
        if (utils.falsey(view.layout)) {
          view.layout = 'empty';
        }
        next();
        return;
      }

      if (view.isType('partial')) {
        view.options.layout = null;
        view.data.layout = null;
        view.layout = null;
        if (typeof view.partialLayout === 'string') {
          view.layout = view.partialLayout;
        }
      }
      next();
    });

    // remove or rename template prefixes before writing files to the file system
    var regex = app.options.templatePathRegex || /./;
    app.templates.preWrite(regex, utils.renameFile(app));
    app.templates.onLoad(regex, function(view, next) {
      var userDefined = app.home('templates', view.basename);
      if (fs.existsSync(userDefined)) {
        view.contents = fs.readFileSync(userDefined);
      }
      utils.stripPrefixes(view);
      utils.parser.parse(view, next);
    });

    // "noop" layout
    app.layout('empty', {content: '{% body %}'});

    // create collections defined on the options
    if (utils.isObject(app.options.create)) {
      for (var key in app.options.create) {
        if (!app[key]) {
          app.create(key, app.options.create[key]);
        } else {
          app[key].option(app.options.create[key]);
        }
      }
    }

    // pass the plugin to sub-generators
    return plugin;
  };
}

/**
 * Expose `plugin` function so that verb-collections
 * can be run as a global generator
 */

module.exports = collections();

/**
 * Expose `collection` function so that options can be passed
 */

module.exports.create = collections;
