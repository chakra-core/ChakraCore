'use strict';

/**
 * module dependencies
 */

var Templates = require('templates');
var utils = require('./utils');

/**
 * Create an `assemble` application. This is the main function exported
 * by the assemble module.
 *
 * ```js
 * var assemble = require('assemble');
 * var app = assemble();
 * ```
 * @param {Object} `options` Optionally pass default options to use.
 * @api public
 */

function Assemble(options) {
  if (!(this instanceof Assemble)) {
    return new Assemble(options);
  }
  Templates.call(this, options);
  this.is('assemble');
  this.initCore();
}

/**
 * Inherit `Templates`
 */

Templates.extend(Assemble);
Templates.bubble(Assemble);

/**
 * Load core plugins
 */

Assemble.prototype.initCore = function() {
  Assemble.initCore(this);
};

/**
 * Load core plugins
 */

Assemble.initCore = function(app) {
  Assemble.emit('preInit', app);
  Assemble.initPlugins(app);
  Assemble.emit('init', app);
};

/**
 * Load core plugins
 */

Assemble.initPlugins = function(app) {
  app.use(utils.tasks(app.name));
  app.use(utils.streams());
  app.use(utils.render());
  app.use(utils.fs());
};

/**
 * Expose the `Assemble` constructor
 */

module.exports = Assemble;
