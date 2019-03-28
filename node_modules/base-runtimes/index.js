/*!
 * base-runtimes <https://github.com/jonschlinkert/base-runtimes>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var utils = require('./utils');

module.exports = function(config) {
  config = config || {};

  return function baseRuntimes(app) {
    if (!utils.isValid(app, 'base-runtimes')) return;
    var time = new utils.Time();
    var log = utils.log;

    this.on('starting', function(build) {
      // create build context
      var ctx = {app: build};
      ctx.key = toKey(namespace(build));
      ctx.event = 'starting';
      time.start(ctx.key);
      ctx.time = '';
      ctx.isBuild = true;
      ctx.isSilent = silent(app, build, null);

      if (app.hasListeners('build')) {
        app.emit('build', ctx.event, ctx);
      } else if (app.base.hasListeners('build')) {
        app.base.emit('build', ctx.event, ctx);
      } else if (!ctx.isSilent) {
        console.error(log.timestamp, ctx.event, ctx.key, log.red(ctx.time));
      }
    });

    this.on('finished', function(build, run) {
      // create build context
      var ctx = {app: build};
      ctx.key = toKey(namespace(build));
      ctx.time = time.end(ctx.key);
      ctx.event = 'finished';
      ctx.isBuild = true;
      ctx.isSilent = silent(app, build, null);

      if (app.hasListeners('build')) {
        app.emit('build', ctx.event, ctx);
      } else if (app.base.hasListeners('build')) {
        app.base.emit('build', ctx.event, ctx);
      } else if (!ctx.isSilent) {
        console.error(log.timestamp, ctx.event, ctx.key, log.red(ctx.time));
      }
    });

    this.on('task:starting', function(task) {
      task.key = toKey(namespace(app), name(task) + ' task');
      time.start('task:' + task.key);
      task.event = 'starting';
      task.time = '';
      task.isTask = true;
      task.isSilent = silent(app, null, task);

      if (app.hasListeners('task')) {
        app.emit('task', task.event, task);
      } else if (app.base.hasListeners('task')) {
        app.base.emit('task', task.event, task);
      } else if (!task.isSilent) {
        console.error(log.timestamp, task.event, task.key, log.red(task.time));
      }
    });

    this.on('task:finished', function(task) {
      task.key = toKey(namespace(app), name(task) + ' task');
      task.time = time.end('task:' + task.key);
      task.event = 'finished';
      task.isTask = true;
      task.isSilent = silent(app, null, task);

      if (app.hasListeners('task')) {
        app.emit('task', task.event, task);
      } else if (app.base.hasListeners('task')) {
        app.base.emit('task', task.event, task);
      } else if (!task.isSilent) {
        console.error(log.timestamp, task.event, task.key, log.red(task.time));
      }
    });

    /**
     * Handle toggling of verbose and silent modes
     */

    function silent(app, build, task) {
      var opts = utils.extend({}, app.base.options, app.options);

      if (build && build.options) {
        opts = utils.extend({}, opts, build.options);
      }
      if (task && task.options) {
        opts = utils.extend({}, opts, task.options);
      }

      var verbose = opts.verbose;
      var silent = opts.silent;

      // handle `verbose` first
      if (typeof verbose === 'function') {
        return verbose(app, build, task);
      }
      if (verbose === true) {
        return false;
      }
      if (task && verbose === 'tasks') {
        return false;
      }
      if (build && verbose === 'build') {
        return false;
      }

      // if not `verbose`, handle `silent`
      if (typeof silent === 'function') {
        return silent(app, build, task);
      }
      if (typeof silent === 'string') {
        silent = [silent];
      }

      if (build && Array.isArray(silent) && isMatch(silent, build.alias)) {
        return true;
      }
      if (task && Array.isArray(silent) && isMatch(silent, task.name)) {
        return true;
      }
      if (silent === true) {
        return true;
      }
    }

    function name(task) {
      return task.name || '';
    }

    function namespace(build) {
      return build.env ? build.env.namespace : (build.namespace || build._name);
    }

    function toKey(namespace, name) {
      var res = '';
      if (namespace) {
        namespace = formatNamespace(namespace);
      }

      if (namespace && name) {
        res = utils.colors.bold(utils.colors.cyan(namespace)) + ':' + utils.colors.yellow(name);

      } else if (namespace) {
        res = utils.colors.bold(utils.colors.cyan(namespace));

      } else if (name) {
        res = utils.colors.cyan(name);
      }
      return res;
    }

    return baseRuntimes;
  };
};

function isMatch(name, val) {
  return utils.mm(val, name).length !== 0;
}

function formatNamespace(name) {
  if (name.indexOf('default.') === 0) {
    return name.slice('default.'.length);
  }

  var segs = name.split('.');
  var len = segs.length;
  var idx = -1
  var res = [];

  while (++idx < len) {
    var next = segs[idx + 1];
    var seg = segs[idx];
    if (next && next === seg) {
      continue;
    }
    res.push(seg);
  }
  return res.join('.');
}
