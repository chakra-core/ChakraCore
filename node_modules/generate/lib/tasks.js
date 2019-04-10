'use strict';

var fs = require('fs');
var utils = require('./utils');

/**
 * Determine the task to run. This is only temporary until we get
 * generators published for handling the built-in tasks.
 */

module.exports = function(app, ctx, argv) {
  var configFile = ctx.env.configFile;
  var defaultTasks = Object.keys(app.generators.defaults.tasks);

  // determine the tasks to run (returns the first value that isn't `["default"]` or `[]`)
  var tasks = utils.getTasks(configFile, [
    argv._, // command line
    ctx.pkgConfig.tasks, // set in package.json
    app.store.get('defaultTasks') // stored user-defined "default" tasks
  ]);

  tasks = tasks || [];

  // no tasks are defined
  if (tasks.length === 0) {
    // if `generator.js` exists in cwd, call it's default task
    if (fs.existsSync(configFile)) {
      return ['default:default'];
    }
    return ['defaults:help'];
  }

  // at least one task was defined
  tasks = tasks.reduce(function(acc, name) {
    if (name.indexOf('help') === 0) {
      return acc.concat('defaults:help');
    }
    if (name.indexOf('list') === 0) {
      return acc.concat('defaults:list');
    }
    if (name.indexOf('tasks') === 0) {
      return acc.concat('defaults:tasks');
    }
    if (name.indexOf('store') === 0) {
      return acc.concat('defaults.' + name);
    }
    acc.push(name);
    return acc;
  }, []);

  if (tasks.length === 1) {
    var task = tasks[0];
    switch (task) {
      case 'default':
        // if a `generator.js` exists in user's cwd, run its default task
        if (fs.existsSync(configFile)) {
          return ['default:default'];
        }
        // fall through
      case 'help':
        return ['defaults:help'];
      case 'new':
        return ['defaults:new'];
      case 'render':
        return ['defaults:render'];
      default:
        return tasks;
    }
  }

  return tasks;
};
