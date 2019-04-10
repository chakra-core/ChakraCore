'use strict';

var Emitter = require('component-emitter');
var flowFactory = require('./flow');
var utils = require('./utils');
var noop = require('./noop');
var Run = require('./run');

/**
 * Task constructor function. Create new tasks.
 *
 * ```
 * var task = new Task({
 *   name: 'site',
 *   deps: ['styles'],
 *   fn: buildSite // defined someplace else
 * });
 * ```
 *
 * @param {Object} `task` Task object used to configure properties on the new Task
 */

function Task(task) {
  if (!(this instanceof Task)) {
    return new Task(task);
  }
  Emitter.call(this);
  if (typeof task === 'undefined') {
    throw new Error('Expected `task` to be an `object` but got `' + typeof task + '`.');
  }
  if (typeof task.name === 'undefined') {
    throw new Error('Expected `task.name` to be a `string` but got `' + typeof task.name + '`.');
  }
  this.app = task.app;
  this.name = task.name;
  this.options = task.options || {};
  this.deps = task.deps || this.options.deps || [];
  this.fn = task.fn || noop;
  this._runs = [];
}

require('util').inherits(Task, Emitter);

/**
 * Setup run meta data to store start and end times and
 * emit starting, finished, and error events.
 *
 * @param  {Function} `cb` Callback function called when task is finished.
 * @return {Function} Function to be used as a `done` function when running a task.
 */

Task.prototype.setupRun = function(cb) {
  var self = this;
  var run = new Run(this._runs.length);
  this._runs.push(run);
  run.start();
  this.emit('starting', this, run);
  return function finishRun(err) {
    run.end();
    if (err) {
      utils.define(err, 'task', self);
      utils.define(err, 'run', run);
      self.emit('error', err);
    } else {
      self.emit('finished', self, run);
    }
    return cb.apply(null, arguments);
  };
};

/**
 * Resolve and execute this task's dependencies.
 *
 * @param  {Function} `cb` Callback function to be called when dependenices are finished running.
 */

Task.prototype.runDeps = function(cb) {
  if (!this.deps.length) {
    return cb();
  }
  var flow = flowFactory(this.options.flow || 'series');
  var fn = flow.call(this.app, this.deps);
  return fn(cb);
};

/**
 * Run a task, capture it's start and end times
 * and emitting 3 possible events:
 *
 *  - starting: just before the task starts with `task` and `run` objects passed
 *  - finished: just after the task finishes with `task` and `run` objects passed
 *  - error: when an error occurs with `err`, `task`, and `run` objects passed.
 *
 * @param  {Function} `cb` Callback function to be called when task finishes running.
 * @return {*} undefined, `Promise` or `Stream` to be used to determine when task finishes running.
 */

Task.prototype.run = function(cb) {

  // exit early when task set not to run.
  if (skip(this)) {
    return cb();
  }

  var self = this;
  var done = this.setupRun(cb);
  this.runDeps(function(err) {
    if (err) return cb(err);
    var results;
    try {
      var fn = self.fn;
      if (utils.isGenerator.fn(fn)) {
        results = utils.co(fn, done);
      } else {
        results = fn.call(self, done);
      }
    } catch (err) {
      return done(err);
    }

    // needed to capture when the actual task is finished running
    if (typeof results !== 'undefined') {
      if (typeof results.on === 'function') {
        results.on('error', done);
        results.on('end', done);
        results.resume();
        return;
      }
      if (typeof results.then === 'function') {
        results.then(function(result) {
          return done(null, result);
        }, done);
        return;
      }
    }
  });
};

function skip(task) {
  if (typeof task.options.run === 'undefined' && typeof task.options.skip === 'undefined') {
    return false;
  }
  if (typeof task.options.run === 'boolean') {
    return task.options.run === false;
  }
  var names = utils.arrayify(task.options.skip);
  return ~names.indexOf(task.name);
}

/**
 * Export Task
 * @type {Task}
 */

module.exports = Task;
