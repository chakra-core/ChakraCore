'use strict';

var compose = require('./compose');
var utils = require('./utils');

/**
 * Run generators, calling `.config.process` first if it exists.
 *
 * @param {String|Array} `name` generator to run
 * @param {Array|String} `tasks` tasks to run
 * @param {Object} `app` Application instance
 * @param {Object} `generator` generator instance
 * @param {Function} next
 */

module.exports = function(app, queued, options, next) {
  var generator = queued.generator;
  var tasks = queued.tasks;

  compose(app, generator);

  if (tasks.length === 1 && !generator.tasks.hasOwnProperty(tasks[0])) {
    if (tasks[0] === 'default') {
      next();
      return;
    }
    var suffix = queued.name !== 'this' ? ('" in generator: "' + queued.name + '"') : '';
    console.error('Cannot find task: "' + tasks[0] + suffix);
    next();
    return;
  }

  utils.merge(generator.options, options);

  var alias = generator.env ? generator.env.alias : generator._name;
  app.emit('generate', alias, queued.tasks, generator);
  if (app._lookup) {
    app.options.lookup = app._lookup;
  }

  // if `base-config` is registered call `.process` first, then run tasks
  if (typeof generator.config !== 'undefined') {
    var config = app.get('cache.config') || {};
    generator.config.process(config, build);
  } else {
    build();
  }

  function build(err) {
    if (err) return done(err);
    generator.build(tasks, done);
  }

  function done(err, result) {
    if (err) {
      err.queue = queued;
      utils.handleError(app, queued.name, next)(err);
    } else {
      next(null, result);
    }
  }
};
