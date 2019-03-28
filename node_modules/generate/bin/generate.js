#!/usr/bin/env node

process.env.GENERATE_CLI = true;
process.on('exit', function() {
  require('set-blocking')(true);
});

var util = require('util');
var path = require('path');
var glob = require('matched');
var gm = require('global-modules');
var debug = require('debug')('generate');
var App = require('..');
var commands = require('../lib/commands');
var plugins = require('../lib/plugins');
var tasks = require('../lib/tasks');
var utils = require('../lib/utils');
var pkg = require('../package');

// Check for and notify if newer version available
var updateNotifier = require('update-notifier');
updateNotifier({pkg}).notify();

var args = utils.args;
var argv = utils.argv;

if (argv.macro) {
  var tasks = argv._.join(', ');
  utils.log.ok('saved macro', utils.log.cyan(argv.macro), utils.log.bold(tasks));
  process.exit();
}

/**
 * Listen for errors on all instances
 */

App.on('generate.preInit', function(app) {
  app.set('cache.argv', argv);
  app.set('cache.args', args);
  app.on('error', function(err) {
    console.error(err);
    process.exit(1);
  });
});

/**
 * Initialize CLI
 */

App.on('generate.postInit', function(app) {
  debug('postInit', app.env);
  app.option(argv);

  var idx = utils.firstIndex(args, ['-D', '--default']);
  if (idx !== -1) {
    args.splice(idx, 1);

    if (args.indexOf('--del') !== -1) {
      var tasks = app.store.get('defaultTasks');
      app.store.del('defaultTasks');
      app.log.warn('deleted default tasks:', tasks);
      app.emit('done');
      process.exit();
    } else {
      app.store.set('defaultTasks', args);
      app.log.success('saved default tasks:', args);
    }
  }
});

/**
 * Initialize Runner
 */

var options = {name: 'generate', configName: 'generator'};

plugins.runner(App, options, argv, function(err, app, runnerContext) {
  if (err) return app.emit('error', err);

  app.set('cache.runnerContext', runnerContext);
  commands(app, runnerContext);

  if (!app.generators.defaults) {
    app.generator('defaults', require('../lib/generator'));
  }

  debug('processing config');
  var ctx = utils.extend({}, runnerContext);
  var config = app.get('cache.config') || {};
  ctx.argv.tasks = [];

  var hooks = glob.sync('generate-hook-*', {cwd: gm});
  for (var i = 0; i < hooks.length; i++) {
    var hook = require(path.resolve(gm, hooks[i]));
    if (typeof hook !== 'function') {
      throw new TypeError(`expected ${hooks[i]} to export a function`);
    }
    app.use(hook(argv, ctx, config));
  }

  app.config.process(config, function(err, config) {
    if (err) return app.emit('error', err);

    // reset `cache.config`
    app.base.cache.config = config;

    app.cli.process(ctx.argv, function(err) {
      if (err) return app.emit('error', err);

      var arr = tasks(app, ctx, argv);
      app.log.success('running tasks:', arr);

      if (ctx.env.configPath) {
        app = app.generator('default', require(ctx.env.configPath));
      }

      app.generate(arr, function(err) {
        if (err) return app.emit('error', err);
      });
    });
  });
});
