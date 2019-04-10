'use strict';

var utils = require('../utils');

/**
 * This plugin decorates static methods onto a class, `App`,
 * for doing instance checks.
 */

module.exports = function(App) {
  if (!App) return;

  /**
   * Static method that returns true if the given value is
   * a `templates` instance (`App`).
   *
   * ```js
   * var templates = require('templates');
   * var app = templates();
   *
   * templates.isApp(templates);
   * //=> false
   *
   * templates.isApp(app);
   * //=> true
   * ```
   * @name .isApp
   * @param {Object} `val` The value to test.
   * @return {Boolean}
   * @api public
   */

  utils.define(App, 'isApp', function isApp(val) {
    return utils.isObject(val) && val.isApp === true;
  });

  /**
   * Static method that returns true if the given value is
   * a templates `Collection` instance.
   *
   * ```js
   * var templates = require('templates');
   * var app = templates();
   *
   * app.create('pages');
   * templates.isCollection(app.pages);
   * //=> true
   * ```
   * @name .isCollection
   * @param {Object} `val` The value to test.
   * @return {Boolean}
   * @api public
   */

  utils.define(App, 'isCollection', function isCollection(val) {
    return utils.isObject(val) && val.isCollection === true;
  });

  /**
   * Static method that returns true if the given value is
   * a templates `Views` instance.
   *
   * ```js
   * var templates = require('templates');
   * var app = templates();
   *
   * app.create('pages');
   * templates.isViews(app.pages);
   * //=> true
   * ```
   * @name .isViews
   * @param {Object} `val` The value to test.
   * @return {Boolean}
   * @api public
   */

  utils.define(App, 'isViews', function isViews(val) {
    return utils.isObject(val) && val.isViews === true;
  });

  /**
   * Static method that returns true if the given value is
   * a templates `List` instance.
   *
   * ```js
   * var templates = require('templates');
   * var List = templates.List;
   * var app = templates();
   *
   * var list = new List();
   * templates.isList(list);
   * //=> true
   * ```
   * @name .isList
   * @param {Object} `val` The value to test.
   * @return {Boolean}
   * @api public
   */

  utils.define(App, 'isList', function isList(val) {
    return utils.isObject(val) && val.isList === true;
  });

  /**
   * Static method that returns true if the given value is
   * a templates `Group` instance.
   *
   * ```js
   * var templates = require('templates');
   * var Group = templates.Group;
   * var app = templates();
   *
   * var group = new Group();
   * templates.isGroup(group);
   * //=> true
   * ```
   * @name .isGroup
   * @param {Object} `val` The value to test.
   * @return {Boolean}
   * @api public
   */

  utils.define(App, 'isGroup', function isGroup(val) {
    return utils.isObject(val) && val.isGroup === true;
  });

  /**
   * Static method that returns true if the given value is
   * a templates `View` instance.
   *
   * ```js
   * var templates = require('templates');
   * var app = templates();
   *
   * templates.isView('foo');
   * //=> false
   *
   * var view = app.view('foo', {content: '...'});
   * templates.isView(view);
   * //=> true
   * ```
   * @name .isView
   * @param {Object} `val` The value to test.
   * @return {Boolean}
   * @api public
   */

  utils.define(App, 'isView', function isView(val) {
    return utils.isObject(val) && val.isView === true;
  });

  /**
   * Static method that returns true if the given value is
   * a templates `Item` instance.
   *
   * ```js
   * var templates = require('templates');
   * var app = templates();
   *
   * templates.isItem('foo');
   * //=> false
   *
   * var view = app.view('foo', {content: '...'});
   * templates.isItem(view);
   * //=> true
   * ```
   * @name .isItem
   * @param {Object} `val` The value to test.
   * @return {Boolean}
   * @api public
   */

  utils.define(App, 'isItem', function isItem(val) {
    return utils.isObject(val) && val.isItem === true;
  });

  /**
   * Static method that returns true if the given value is
   * a vinyl `File` instance.
   *
   * ```js
   * var File = require('vinyl');
   * var templates = require('templates');
   * var app = templates();
   *
   * var view = app.view('foo', {content: '...'});
   * templates.isVinyl(view);
   * //=> true
   *
   * var file = new File({path: 'foo', contents: new Buffer('...')});
   * templates.isVinyl(file);
   * //=> true
   * ```
   * @name .isVinyl
   * @param {Object} `val` The value to test.
   * @return {Boolean}
   * @api public
   */

  utils.define(App, 'isVinyl', function isVinyl(val) {
    return utils.isObject(val) && val._isVinyl === true;
  });
};

