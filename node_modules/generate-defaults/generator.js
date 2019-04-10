'use strict';

var utils = require('./utils');
var answer;

module.exports = function plugin(app) {
  if (!utils.isValid(app, 'generate-defaults')) return;
  var project;

  /**
   * Plugins
   */

  app.use(require('generate-data'));
  app.use(utils.middleware());
  app.use(utils.questions());

  /**
   * Merge package.json object onto the `project` property on the context
   */

  Object.defineProperty(app.cache.data, 'project', {
    set: function(val) {
      project = val;
    },
    get: function() {
      return project || (project = utils.extend({}, app.pkg.data));
    }
  });

  /**
   * Engine
   */

  if (!app.getEngine('*')) {
    app.engine('*', require('engine-base'), app.option('engineOpts'));
  }

  /**
   * Helpers
   */

  app.helpers(require('template-helpers')());

  /**
   * Tasks
   */

  app.task('check-directory', function(cb) {
    if (utils.isEmpty(app.cwd) || answer === true) {
      cb();
      return;
    }

    app.confirm('directory', 'The current working directory has existing files in it, are you sure you want to proceed?', {
      default: false
    });

    app.ask('directory', function(err, answers) {
      if (err) {
        cb(err);
        return;
      }
      if (answers.directory === false) {
        app.log.warn(app.log.yellow('Got it, exiting process'));
        process.exit();
      } else {
        answer = true;
        cb();
      }
    });
  });

  return plugin;
};
